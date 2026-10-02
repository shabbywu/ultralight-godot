#pragma once
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <memory>
#include <set>
#include <mutex>
#include <vector>
#include <ulbind17/detail/function/cpp_function.hpp>
#include <ulbind17/ulbind17.hpp>

namespace gdbind {
class godot_callable : public ulbind17::detail::generic_function {
  public:
    godot_callable(JSContextRef context, godot::Callable callback) : callable(callback), context(context) {
        std::lock_guard lock(registry_mutex);
        registry.insert(this);
    }
    ~godot_callable() {
        std::lock_guard lock(registry_mutex);
        registry.erase(this);
    }
    // JSC may retain native wrappers until VM shutdown, after GDScript has
    // already destroyed its mutexes. Release Godot Callables with their View.
    static void release_context(JSContextRef context) {
        std::vector<godot::Callable> released;
        {
            std::lock_guard lock(registry_mutex);
            for (auto *function : registry) {
                if (function->context != context) continue;
                released.push_back(function->callable);
                function->callable = godot::Callable();
            }
        } // Release closures outside the registry lock; destruction can re-enter.

    }

  protected:
    godot::Callable callable;
    JSContextRef context;
    inline static std::set<godot_callable *> registry;
    inline static std::mutex registry_mutex;

  public:
    inline std::function<Caller> build_caller_impl();
    virtual std::function<Caller> get_caller_impl() {
        return build_caller_impl();
    }

    virtual Caller *get_static_caller() {
        return &caller;
    }
    virtual int get_nargs() {
        return callable.get_argument_count();
    }

  public:
    static JSValueRef caller(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                             const JSValueRef arguments[], JSValueRef *exception) {
        auto h = (ulbind17::detail::PrivateDataHolder<godot_callable> *)JSObjectGetPrivate(function);
        return (**h).get_caller_impl()(ctx, function, thisObject, argumentCount, arguments, exception);
    }
};

class NativeFunction : public ulbind17::detail::NativeFunction<godot::Callable> {
  protected:
    using Holder = ulbind17::detail::JSHolder<JSObjectRef>;
    std::shared_ptr<Holder> holder;

  public:
    NativeFunction(JSContextRef ctx, std::shared_ptr<ulbind17::detail::generic_function> func)
        : holder(std::make_shared<Holder>(ctx, make_instance(ctx, func))) {
    }

  public:
    JSObjectRef rawref() const {
        return holder->value;
    }
};

} // namespace gdbind

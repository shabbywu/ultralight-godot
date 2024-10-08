#pragma once
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <memory>
#include <ulbind17/detail/function/cpp_function.hpp>
#include <ulbind17/ulbind17.hpp>

namespace gdbind {
class godot_callable : public ulbind17::detail::generic_function {
  public:
    godot_callable(godot::Callable callbale) : callable(callbale) {
    }

  protected:
    godot::Callable callable;

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

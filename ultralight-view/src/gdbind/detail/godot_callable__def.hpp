#pragma once
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <memory>
#include <ulbind17/detail/function/cpp_function.hpp>

namespace gdbind {
class godot_callable : public ulbind17::detail::generic_function {
  public:
    godot_callable(godot::Callable callbale) : callable(callbale) {
    }

  protected:
    godot::Callable callable;

  public:
    inline std::function<Caller> build_caller_impl();
    virtual inline std::function<Caller> get_caller_impl() {
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

} // namespace gdbind

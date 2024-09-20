#pragma once
#include "cast.hpp"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <memory>
#include <ulbind17/ulbind17.hpp>

namespace gdbind {
class godot_callbale : public ulbind17::detail::generic_function {
  public:
    godot_callbale(godot::Callable callbale) : callable(callbale) {
    }

  protected:
    godot::Callable callable;

  private:
    template <class Func, typename Return, typename... Args>
    std::function<Caller> build_caller(Func &&f, Return (*signature)(Args...) = nullptr,
                                       typename std::enable_if_t<std::is_void_v<Return>, Return> * = nullptr) {
        return std::function<Caller>([f, this](JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                               size_t argumentCount, const JSValueRef arguments[],
                                               JSValueRef *exception) -> JSValueRef {
            std::vector<JSValueRef> realArguments;
            if (withThisObject) {
                realArguments.push_back(
                    generic_cast<JSObjectRef, JSValueRef>(ctx, std::forward<JSObjectRef>(thisObject)));
                for (int i = 0; i < argumentCount; i++) {
                    realArguments.push_back(arguments[i]);
                }
            }
            if (argumentCount < nargs) {
                if (exception) {
                    auto message = adopt(JSStringCreateWithUTF8CString("argusments not enough"));
                    *exception = JSValueMakeString(ctx, message.get());
                }
                return JSValueMakeNull(ctx);
            }

            auto args = load_args<0, std::tuple<Args...>>::load(ctx, withThisObject ? realArguments.data() : arguments);
            try {
                if (holder->functor) {
                    auto func = (std::function<Return(Args...)> *)holder->data[0];
                    std::apply(*func, args);
                } else {
                    auto func = (Return(*)(Args...))holder->data[0];
                    std::apply(*func, args);
                }
            }
            catch (const std::exception &e) {
                if (exception) {
                    auto message = adopt(JSStringCreateWithUTF8CString(e.what()));
                    *exception = JSValueMakeString(ctx, message.get());
                }
            }
            return JSValueMakeNull(ctx);
        });
    }

  public:
    virtual std::function<Caller> get_caller_impl() {
        return [this](JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                      const JSValueRef arguments[], JSValueRef *exception) -> JSValueRef {
            godot::Array gd_arguments;
            for (int i = 0; i < argumentCount; i++) {
                gd_arguments.push_back(ulbind17::detail::generic_cast<JSValueRef, godot::Variant>(
                    ctx, std::forward<const JSValueRef>(arguments[i])));
            }
            try {
                auto result = this->callable.callv(gd_arguments);
                return ulbind17::detail::generic_cast<godot::Variant, JSValueRef>(ctx,
                                                                                  std::forward<godot::Variant>(result));
            }
            catch (const std::exception &e) {
                if (exception) {
                    auto message = adopt(JSStringCreateWithUTF8CString(e.what()));
                    *exception = JSValueMakeString(ctx, message.get());
                }
            }
            return JSValueMakeNull(ctx);
        };
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
        auto h = (ulbind17::detail::PrivateDataHolder<godot_callbale> *)JSObjectGetPrivate(function);
        return (**h).get_caller_impl()(ctx, function, thisObject, argumentCount, arguments, exception);
    }
};

class NativeFunction : public ulbind17::detail::NativeFunction<Callable> {
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

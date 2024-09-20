#pragma once
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <memory>
#include <ulbind17/ulbind17.hpp>

namespace gdbind {

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

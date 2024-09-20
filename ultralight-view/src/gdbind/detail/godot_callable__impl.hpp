#pragma once
#include "cast.hpp"
#include "godot_callable__def.hpp"

namespace gdbind {
inline std::function<godot_callable::Caller> godot_callable::build_caller_impl() {
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

} // namespace gdbind

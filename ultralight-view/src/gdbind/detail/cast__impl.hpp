#pragma once
#include "cast__def.hpp"
#include "ulbind17/cast.hpp"

#include "godot_callable__def.hpp"
#include "native_function.hpp"

using namespace godot;

namespace ulbind17 {
namespace detail {
JSValueRef generic_cast(JSContextRef ctx, Variant value) {
    switch (value.get_type()) {
    case Variant::Type::NIL:
        return JSValueMakeNull(ctx);
        break;
    case Variant::Type::BOOL: {
        bool v(value);
        return generic_cast<bool, JSValueRef>(ctx, std::move(v));
    }
    case Variant::Type::INT: {
        int v(value);
        return generic_cast<int, JSValueRef>(ctx, std::move(v));
    }

    case Variant::Type::FLOAT: {
        float v(value);
        return generic_cast<float, JSValueRef>(ctx, std::move(v));
    }
    case Variant::Type::STRING: {
        godot::String s(value);
        return generic_cast<std::string, JSValueRef>(ctx, std::move(std::string(s.utf8().ptr())));
    }
    case Variant::Type::CALLABLE: {
        auto func = std::make_shared<gdbind::godot_callable>(value);
        return gdbind::NativeFunction(ctx, func).rawref();
    }
    case Variant::Type::DICTIONARY: {
        // TODO: support cast godot::Dictionary to js object
    }
    case Variant::Type::ARRAY: {
        // TODO: support cast godot:Array to js array
    }
    default:
        break;
    }
    return JSValueMakeNull(ctx);
}
Variant generic_cast(JSContextRef ctx, JSValueRef value) {
    switch (JSValueGetType(ctx, value)) {
    case JSType::kJSTypeUndefined:
    case JSType::kJSTypeNull:
        return nullptr;
    case JSType::kJSTypeBoolean:
        return generic_cast<JSValueRef, bool>(ctx, std::move(value));
    case JSType::kJSTypeNumber:
        return generic_cast<JSValueRef, double>(ctx, std::move(value));
    case JSType::kJSTypeString: {
        auto s = generic_cast<JSValueRef, std::string>(ctx, std::move(value));
        return godot::String(s.data());
    }
    }
    return nullptr;
}
} // namespace detail
} // namespace ulbind17

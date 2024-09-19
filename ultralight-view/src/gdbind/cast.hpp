#pragma once
#include "ulbind17/cast.hpp"
#include <godot_cpp/variant/variant.hpp>
#include <type_traits>

#include "Callable.hpp"

using namespace godot;

namespace ulbind17 {
namespace detail {
// cast Variant to JSValue
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, Variant>> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, JSValueRef>> * = nullptr>
ToType generic_cast(JSContextRef ctx, FromType &&value) {
    switch (value.get_type()) {
    case Variant::Type::NIL:
        return JSValueMakeNull(ctx);
        break;
    case Variant::Type::BOOL: {
        bool v(value);
        return generic_cast<bool, ToType>(ctx, std::move(v));
    }
    case Variant::Type::INT: {
        int v(value);
        return generic_cast<int, ToType>(ctx, std::move(v));
    }

    case Variant::Type::FLOAT: {
        float v(value);
        return generic_cast<float, ToType>(ctx, std::move(v));
    }
    case Variant::Type::STRING: {
        godot::String s(value);
        return generic_cast<std::string, ToType>(ctx, std::move(std::string(s.utf8().ptr())));
    }
    case Variant::Type::CALLABLE: {
        auto func = std::make_shared<gdbind::godot_callbale>(value);
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
};

// cast JSValue to Variant
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, JSValueRef>> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, Variant>> * = nullptr>
ToType generic_cast(JSContextRef ctx, FromType &&value) {
    switch (JSValueGetType(ctx, value)) {
    case JSType::kJSTypeUndefined:
    case JSType::kJSTypeNull:
        return nullptr;
    case JSType::kJSTypeBoolean:
        return generic_cast<FromType, bool>(ctx, std::move(value));
    case JSType::kJSTypeNumber:
        return generic_cast<FromType, double>(ctx, std::move(value));
    case JSType::kJSTypeString: {
        auto s = generic_cast<FromType, std::string>(ctx, std::move(value));
        return godot::String(s.data());
    }
    }
    return nullptr;
};

template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, JSValueRef>> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, Variant>> * = nullptr>
ToType generic_cast(JSContextRef ctx, const FromType &&value) {
    switch (JSValueGetType(ctx, value)) {
    case JSType::kJSTypeUndefined:
    case JSType::kJSTypeNull:
        return nullptr;
    case JSType::kJSTypeBoolean:
        return generic_cast<const FromType, bool>(ctx, std::move(value));
    case JSType::kJSTypeNumber:
        return generic_cast<const FromType, double>(ctx, std::move(value));
    case JSType::kJSTypeString: {
        auto s = generic_cast<const FromType, std::string>(ctx, std::move(value));
        return godot::String(s.data());
    }
    case JSType::kJSTypeObject: {
        // TODO: support conevrt js array to godot::Array
        // TODO: support conevrt js object to godot::JSON
    }
    }
    return nullptr;
};

} // namespace detail
} // namespace ulbind17

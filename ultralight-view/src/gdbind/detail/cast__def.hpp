#pragma once
#include <JavaScriptCore/JSRetainPtr.h>
#include <godot_cpp/variant/variant.hpp>
#include <type_traits>

using namespace godot;

namespace ulbind17 {
namespace detail {
JSValueRef generic_cast(JSContextRef ctx, Variant value);
Variant generic_cast(JSContextRef ctx, JSValueRef value);
} // namespace detail
} // namespace ulbind17

namespace ulbind17 {
namespace detail {
// cast Variant to JSValue
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, Variant>> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, JSValueRef>> * = nullptr>
ToType generic_cast(JSContextRef ctx, FromType &&value) {
    return generic_cast(ctx, value);
}

// cast JSValue to Variant
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, JSValueRef>> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, Variant>> * = nullptr>
ToType generic_cast(JSContextRef ctx, FromType &&value) {
    return generic_cast(ctx, value);
}

template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, JSValueRef>> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, Variant>> * = nullptr>
ToType generic_cast(JSContextRef ctx, const FromType &&value) {
    return generic_cast(ctx, value);
}

} // namespace detail
} // namespace ulbind17

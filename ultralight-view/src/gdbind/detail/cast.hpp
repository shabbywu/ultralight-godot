#pragma once
#include "JavaScriptBridge__def.hpp"
#include "js_string.hpp"
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <algorithm>

namespace gdbind {
inline godot::Variant BridgeContext::read(JSValueRef value, std::vector<JSObjectRef> &parents) {
    auto ctx = runtime_->context();
    if (!value) throw std::runtime_error("Empty JavaScript value");
    Root root(ctx, value);
    switch (JSValueGetType(ctx, value)) {
    case kJSTypeUndefined:
    case kJSTypeNull: return godot::Variant();
    case kJSTypeBoolean: return JSValueToBoolean(ctx, value);
    case kJSTypeNumber: return JSValueToNumber(ctx, value, nullptr);
    case kJSTypeString: {
        auto text = JSValueToStringCopy(ctx, value, nullptr);
        auto result = godot_string(text);
        JSStringRelease(text);
        return result;
    }
    case kJSTypeObject: break;
    default: throw std::runtime_error("JavaScript value type is not supported by Godot Variant");
    }
    auto object = JSValueToObject(ctx, value, nullptr);
    if (auto id = runtime_->unwrap<godot::ObjectID>(value)) return godot::ObjectDB::get_instance(*id);
    if (runtime_->isPromise(value)) return readPromise(object);
    if (JSObjectIsFunction(ctx, object)) return readFunction(object);
    if (parents.size() >= 64 || std::find(parents.begin(), parents.end(), object) != parents.end())
        throw std::runtime_error("Cyclic or excessively nested JavaScript data");
    parents.push_back(object);
    JSValueRef exception = nullptr;
    if (JSValueIsArray(ctx, value)) {
        JSString length{godot::String("length")};
        auto length_value = JSObjectGetProperty(ctx, object, length, &exception);
        check(exception);
        auto size = JSValueToNumber(ctx, length_value, &exception);
        check(exception);
        godot::Array result;
        result.resize(static_cast<int64_t>(size));
        for (int64_t i = 0; i < result.size(); ++i) {
            auto item = JSObjectGetPropertyAtIndex(ctx, object, i, &exception);
            check(exception);
            result[i] = read(item, parents);
        }
        parents.pop_back();
        return result;
    }
    godot::Dictionary result;
    auto keys = JSObjectCopyPropertyNames(ctx, object);
    try {
        for (size_t i = 0; i < JSPropertyNameArrayGetCount(keys); ++i) {
            auto name = JSPropertyNameArrayGetNameAtIndex(keys, i);
            auto item = JSObjectGetProperty(ctx, object, name, &exception);
            check(exception);
            result[godot_string(name)] = read(item, parents);
        }
    } catch (...) {
        JSPropertyNameArrayRelease(keys);
        throw;
    }
    JSPropertyNameArrayRelease(keys);
    parents.pop_back();
    return result;
}

inline godot::Variant BridgeContext::toGodot(JSValueRef value) {
    std::vector<JSObjectRef> parents;
    return read(value, parents);
}

inline JSValueRef BridgeContext::toJS(const godot::Variant &value, unsigned depth) {
    auto ctx = runtime_->context();
    if (!active() || depth >= 64) throw std::runtime_error("Invalid context or excessively nested Godot data");
    switch (value.get_type()) {
    case godot::Variant::NIL: return JSValueMakeNull(ctx);
    case godot::Variant::BOOL: return JSValueMakeBoolean(ctx, bool(value));
    case godot::Variant::INT: return JSValueMakeNumber(ctx, double(int64_t(value)));
    case godot::Variant::FLOAT: return JSValueMakeNumber(ctx, double(value));
    case godot::Variant::STRING: {
        JSString text{godot::String(value)};
        return JSValueMakeString(ctx, text);
    }
    case godot::Variant::CALLABLE: return writeCallable(godot::Callable(value));
    case godot::Variant::OBJECT: return writeObject(value);
    case godot::Variant::ARRAY: {
        godot::Array values = value;
        auto array = JSObjectMakeArray(ctx, 0, nullptr, nullptr);
        Root root(ctx, array);
        JSValueRef exception = nullptr;
        for (int64_t i = 0; i < values.size(); ++i) {
            JSObjectSetPropertyAtIndex(ctx, array, i, toJS(values[i], depth + 1), &exception);
            check(exception);
        }
        return array;
    }
    case godot::Variant::DICTIONARY: {
        godot::Dictionary values = value;
        auto object = JSObjectMake(ctx, nullptr, nullptr);
        Root root(ctx, object);
        auto keys = values.keys();
        JSValueRef exception = nullptr;
        for (int64_t i = 0; i < keys.size(); ++i) {
            JSString key{godot::String(keys[i])};
            JSObjectSetProperty(ctx, object, key, toJS(values[keys[i]], depth + 1), kJSPropertyAttributeNone, &exception);
            check(exception);
        }
        return object;
    }
    default: throw std::runtime_error("Godot Variant type is not supported by JavaScript");
    }
}
} // namespace gdbind

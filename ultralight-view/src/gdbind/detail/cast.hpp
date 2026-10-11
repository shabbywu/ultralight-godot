#pragma once
#include "JavaScriptBridge__def.hpp"
#include "js_string.hpp"
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <limits>

namespace gdbind {
inline godot::Variant BridgeContext::read(JSValueRef value, ulbind17::jsc::Traversal &traversal) {
    auto ctx = runtime_->context();
    if (!value) throw std::runtime_error("Empty JavaScript value");
    Root root(ctx, value);
    switch (JSValueGetType(ctx, value)) {
    case kJSTypeUndefined:
    case kJSTypeNull: return godot::Variant();
    case kJSTypeBoolean: return runtime_->readBoolean(value);
    case kJSTypeNumber: return runtime_->readNumber(value);
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
    auto guard = traversal.enter(object);
    if (JSValueIsArray(ctx, value)) {
        auto size = runtime_->length(value);
        godot::Array result;
        if (result.resize(static_cast<int64_t>(size)) != godot::OK)
            throw std::runtime_error("Could not allocate Godot array");
        for (int64_t i = 0; i < result.size(); ++i) {
            result[i] = read(runtime_->at(object, static_cast<uint32_t>(i)), traversal);
        }
        return result;
    }
    godot::Dictionary result;
    // Preserve the existing JSC enumeration (including prototype properties),
    // rather than changing Variant conversion to Object.keys() semantics.
    auto keys = JSObjectCopyPropertyNames(ctx, object);
    try {
        runtime_->context(); // Enumeration hooks may navigate or close the View.
        for (size_t i = 0; i < JSPropertyNameArrayGetCount(keys); ++i) {
            auto name = JSPropertyNameArrayGetNameAtIndex(keys, i);
            auto item = runtime_->get(object, ulbind17::jsc::utf8(name));
            result[godot_string(name)] = read(item, traversal);
        }
    } catch (...) {
        JSPropertyNameArrayRelease(keys);
        throw;
    }
    JSPropertyNameArrayRelease(keys);
    return result;
}

inline godot::Variant BridgeContext::toGodot(JSValueRef value) {
    ulbind17::jsc::Traversal traversal;
    return read(value, traversal);
}

inline JSValueRef BridgeContext::toJS(const godot::Variant &value, unsigned depth) {
    auto ctx = runtime_->context();
    if (!active() || depth >= 64) throw std::runtime_error("Invalid context or excessively nested Godot data");
    switch (value.get_type()) {
    case godot::Variant::NIL: return runtime_->makeNull();
    case godot::Variant::BOOL: return runtime_->makeBoolean(bool(value));
    case godot::Variant::INT: return runtime_->makeNumber(double(int64_t(value)));
    case godot::Variant::FLOAT: return runtime_->makeNumber(double(value));
    case godot::Variant::STRING: {
        JSString text{godot::String(value)};
        return JSValueMakeString(ctx, text);
    }
    case godot::Variant::CALLABLE: return writeCallable(godot::Callable(value));
    case godot::Variant::OBJECT: return writeObject(value);
    case godot::Variant::ARRAY: {
        godot::Array values = value;
        if (static_cast<uint64_t>(values.size()) > (std::numeric_limits<uint32_t>::max)())
            throw std::runtime_error("Godot array exceeds JavaScript index range");
        auto array = runtime_->makeArray();
        Root root(ctx, array);
        for (int64_t i = 0; i < values.size(); ++i) {
            runtime_->setAt(array, static_cast<uint32_t>(i), toJS(values[i], depth + 1));
        }
        return array;
    }
    case godot::Variant::DICTIONARY: {
        godot::Dictionary values = value;
        auto object = runtime_->makePlainObject();
        Root root(ctx, object);
        auto keys = values.keys();
        for (int64_t i = 0; i < keys.size(); ++i) {
            runtime_->set(object, godot_utf8(godot::String(keys[i])), toJS(values[keys[i]], depth + 1));
        }
        return object;
    }
    default: throw std::runtime_error("Godot Variant type is not supported by JavaScript");
    }
}
} // namespace gdbind

#pragma once
#include "cast__def.hpp"
#include "godot_callable__def.hpp"
#include "godot_object.hpp"
#include "js_callable.hpp"
#include "js_callable_trampoline.hpp"
#include "ulbind17/ulbind17.hpp"
#include <godot_cpp/variant/utility_functions.hpp>
#include <string>

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
        Object out(ctx);
        godot::Dictionary d(value);
        auto keys = d.keys();
        for (int i = 0; i < keys.size(); i++) {
            godot::String key = keys[i];
            out.set(std::string(key.utf8().ptr()), generic_cast<Variant, JSValueRef>(ctx, d.get(key, nullptr)));
        }
        return out.rawref();
    }
    case Variant::Type::ARRAY: {
        Array out(ctx);
        godot::Array a(value);
        for (int i = 0; i < a.size(); i++) {
            out.set(i, generic_cast<Variant, JSValueRef>(ctx, std::move(a[i])));
        }
        return out.rawref();
    }
    case Variant::Type::OBJECT: {
        godot::Object *instance(value);
        return generic_cast<godot::Object *, JSValueRef>(ctx, std::move(instance));
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
    case JSType::kJSTypeObject: {
        if (JSValueIsArray(ctx, value)) {
            godot::Array out;
            Array a(ctx, JSValueToObject(ctx, value, nullptr));
            out.resize(a.size());
            for (int i = 0; i < a.size(); i++) {
                out[i] = a.get<Variant>(i);
            }
            return out;
        } else {
            Object o(ctx, JSValueToObject(ctx, value, nullptr));
            if (o.isFunction()) {
                auto out = memnew(gdbind::JavascrtipCallableTrampoline);
                out->callable = new gdbind::JavascriptCallable(o);
                return Callable(out, "trampoline");
            } else {
                godot::Dictionary out;
                auto keys = o.keys();
                for (auto key : keys) {
                    out[key.data()] = o.get<Variant>(key);
                }
                return out;
            }
        }
    }
    }
    return nullptr;
}
} // namespace detail
} // namespace ulbind17

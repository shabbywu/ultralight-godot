#pragma once
#include "JavaScriptBridge__def.hpp"
#include "js_promise__def.hpp"
#include "js_string.hpp"
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <algorithm>

namespace gdbind {
// Metadata only: never read property values while querying or enumerating.
inline std::vector<std::string> object_property_names(godot::Object *object) {
    std::vector<std::string> result;
    const auto properties = object->get_property_list();
    for (int64_t i = 0; i < properties.size(); ++i) {
        godot::Dictionary property = properties[i];
        const auto usage = int64_t(property.get("usage", 0));
        if (usage & (godot::PROPERTY_USAGE_CATEGORY | godot::PROPERTY_USAGE_GROUP |
                     godot::PROPERTY_USAGE_SUBGROUP | godot::PROPERTY_USAGE_INTERNAL)) continue;
        auto name = godot_utf8(godot::String(property.get("name", "")));
        if (!name.empty() && std::find(result.begin(), result.end(), name) == result.end())
            result.push_back(std::move(name));
    }
    return result;
}

inline JSValueRef BridgeContext::writeObject(godot::Object *object, bool query_members) {
    auto ctx = runtime_->context();
    if (!object) return JSValueMakeNull(ctx);
    if (auto promise = godot::Object::cast_to<JavaScriptPromise>(object)) return promise->value(this);
    godot::ObjectID id(object->get_instance_id());
    ulbind17::jsc::Bridge::ObjectHooks hooks;
    hooks.get = [weak = weak_from_this(), id](JSContextRef, JSStringRef name) -> JSValueRef {
        auto state = weak.lock();
        auto *instance = godot::ObjectDB::get_instance(id);
        if (!state || !state->active() || !instance)
            throw std::runtime_error("Godot object is no longer valid");
        auto property = godot_string(name);
        return state->toJS(instance->has_method(property)
            ? godot::Variant(godot::Callable(instance, property)) : instance->get(property));
    };
    hooks.set = [weak = weak_from_this(), id](JSContextRef, JSStringRef name, JSValueRef value) {
        auto state = weak.lock();
        auto *instance = godot::ObjectDB::get_instance(id);
        if (!state || !state->active() || !instance)
            throw std::runtime_error("Godot object is no longer valid");
        auto property = godot_string(name);
        auto converted = state->toGodot(value);
        // JS getters executed by conversion can release the Godot object.
        instance = godot::ObjectDB::get_instance(id);
        if (!state->active() || !instance)
            throw std::runtime_error("Godot object is no longer valid");
        instance->set(property, converted);
        return true;
    };
    if (query_members) {
        hooks.has = [weak = weak_from_this(), id](JSContextRef, JSStringRef name) {
            auto state = weak.lock();
            auto *instance = godot::ObjectDB::get_instance(id);
            if (!state || !state->active() || !instance) return false;
            if (instance->has_method(godot_string(name))) return true;
            auto names = object_property_names(instance);
            return state->active() && godot::ObjectDB::get_instance(id) &&
                   std::find(names.begin(), names.end(), ulbind17::jsc::utf8(name)) != names.end();
        };
        hooks.keys = [weak = weak_from_this(), id](JSContextRef) {
            auto state = weak.lock();
            auto *instance = godot::ObjectDB::get_instance(id);
            if (!state || !state->active() || !instance) return std::vector<std::string>{};
            auto names = object_property_names(instance);
            return state->active() && godot::ObjectDB::get_instance(id) ? names : std::vector<std::string>{};
        };
    }
    return runtime_->makeObject(id, std::move(hooks));
}
} // namespace gdbind

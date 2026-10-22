#pragma once
#include "JavaScriptBridge__def.hpp"
#include "js_promise__def.hpp"
#include "js_string.hpp"
#include <algorithm>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace gdbind {
// Metadata only: never read property values while querying or enumerating.
inline std::vector<std::string> object_property_names(godot::Object *object) {
    std::vector<std::string> result;
    const auto properties = object->get_property_list();
    for (int64_t i = 0; i < properties.size(); ++i) {
        godot::Dictionary property = properties[i];
        const auto usage = int64_t(property.get("usage", 0));
        if (usage & (godot::PROPERTY_USAGE_CATEGORY | godot::PROPERTY_USAGE_GROUP | godot::PROPERTY_USAGE_SUBGROUP |
                     godot::PROPERTY_USAGE_INTERNAL))
            continue;
        auto name = godot_utf8(godot::String(property.get("name", "")));
        if (!name.empty() && std::find(result.begin(), result.end(), name) == result.end())
            result.push_back(std::move(name));
    }
    return result;
}

template <typename... Args> inline godot::Variant managedCall(godot::Object *object, const char *method, Args... args) {
    godot::Dictionary result = object->call(method, args...);
    if (!bool(result.get("ok", false)))
        throw std::runtime_error(godot_utf8(godot::String(result.get("error", "Managed bridge failed"))));
    return result.get("value", godot::Variant());
}
inline JSValueRef BridgeContext::writeObject(godot::Object *object, bool query_members) {
    auto ctx = runtime_->context();
    if (!object)
        return JSValueMakeNull(ctx);
    if (godot::Object::cast_to<UltralightUndefined>(object))
        return JSValueMakeUndefined(ctx);
    if (auto value = godot::Object::cast_to<UltralightJSValue>(object))
        return value->value(this);
    if (auto task = godot::Object::cast_to<UltralightTask>(object))
        return writeTask(godot::Ref<UltralightTask>(task));
    if (auto promise = godot::Object::cast_to<JavaScriptPromise>(object))
        return promise->value(this);
    godot::ObjectID id(object->get_instance_id());
    if (shared_mode_ && object->has_method("_ultralight_kind") &&
        godot::String(object->call("_ultralight_kind")) != "object")
        return writeContainer(object);
    if (auto found = objects_.find(id); (shared_mode_ || enumerate_members_) && found != objects_.end())
        return found->second->value();
    ulbind17::jsc::Bridge::ObjectHooks hooks;
    hooks.get = [weak = weak_from_this(), id,
                 members = id == members_root_ ? allowed_members_ : godot::PackedStringArray()](
                    JSContextRef, JSStringRef name) -> JSValueRef {
        auto state = weak.lock();
        auto *instance = godot::ObjectDB::get_instance(id);
        if (!state || !state->active() || !instance)
            throw std::runtime_error("Godot object is no longer valid");
        auto property = godot_string(name);
        if (!members.is_empty() && !members.has(property))
            throw std::runtime_error("Member is not exported");
        if (instance->has_method("_ultralight_get")) {
            if (!bool(instance->call("_ultralight_has", property)))
                return JSValueMakeUndefined(state->runtime_->context());
            if (bool(instance->call("_ultralight_method", property)))
                return state->runtime_->makeFunction(
                    [weak, id, property](JSContextRef, JSObjectRef, std::span<const JSValueRef> args) -> JSValueRef {
                        auto state = weak.lock();
                        auto *source = godot::ObjectDB::get_instance(id);
                        if (!state || !state->active() || !source)
                            throw std::runtime_error("Binding expired");
                        godot::Array values;
                        for (auto arg : args)
                            values.push_back(state->readShared(arg));
                        auto result = managedCall(source, "_ultralight_call_result", property, values);
                        return state->toJS(result);
                    });
            return state->toJS(managedCall(instance, "_ultralight_get_result", property));
        }
        return state->toJS(instance->has_method(property) ? godot::Variant(godot::Callable(instance, property))
                                                          : instance->get(property));
    };
    hooks.set = [weak = weak_from_this(), id,
                 members = id == members_root_ ? allowed_members_ : godot::PackedStringArray()](
                    JSContextRef, JSStringRef name, JSValueRef value) {
        auto state = weak.lock();
        auto *instance = godot::ObjectDB::get_instance(id);
        if (!state || !state->active() || !instance)
            throw std::runtime_error("Godot object is no longer valid");
        auto property = godot_string(name);
        if (!members.is_empty() && !members.has(property))
            throw std::runtime_error("Member is not exported");
        if (state->read_only_)
            throw std::runtime_error("Binding is read-only");
        auto converted = state->toGodot(value);
        // JS getters executed by conversion can release the Godot object.
        instance = godot::ObjectDB::get_instance(id);
        if (!state->active() || !instance)
            throw std::runtime_error("Godot object is no longer valid");
        if (instance->has_method("_ultralight_set"))
            managedCall(instance, "_ultralight_set_result", property, converted);
        else
            instance->set(property, converted);
        return true;
    };
    if (query_members || enumerate_members_ || shared_mode_ || !allowed_members_.is_empty()) {
        hooks.has = [weak = weak_from_this(), id,
                     members = id == members_root_ ? allowed_members_ : godot::PackedStringArray()](JSContextRef,
                                                                                                    JSStringRef name) {
            auto state = weak.lock();
            auto *instance = godot::ObjectDB::get_instance(id);
            if (!state || !state->active() || !instance)
                return false;
            if (!members.is_empty() && !members.has(godot_string(name)))
                return false;
            if (instance->has_method("_ultralight_has"))
                return bool(instance->call("_ultralight_has", godot_string(name)));
            if (instance->has_method(godot_string(name)))
                return true;
            auto names = object_property_names(instance);
            if (instance->has_method("_ultralight_keys")) {
                names.clear();
                godot::Array keys = managedCall(instance, "_ultralight_keys_result");
                for (int64_t i = 0; i < keys.size(); ++i)
                    names.push_back(godot_utf8(godot::String(keys[i])));
            }
            if (!members.is_empty())
                std::erase_if(names, [&](auto &key) { return !members.has(godot::String::utf8(key.c_str())); });
            return state->active() && godot::ObjectDB::get_instance(id) &&
                   std::find(names.begin(), names.end(), ulbind17::jsc::utf8(name)) != names.end();
        };
        hooks.keys = [weak = weak_from_this(), id,
                      members = id == members_root_ ? allowed_members_ : godot::PackedStringArray()](JSContextRef) {
            auto state = weak.lock();
            auto *instance = godot::ObjectDB::get_instance(id);
            if (!state || !state->active() || !instance)
                return std::vector<std::string>{};
            auto names = object_property_names(instance);
            if (instance->has_method("_ultralight_keys")) {
                names.clear();
                godot::Array keys = managedCall(instance, "_ultralight_keys_result");
                for (int64_t i = 0; i < keys.size(); ++i)
                    names.push_back(godot_utf8(godot::String(keys[i])));
            }
            if (!members.is_empty())
                std::erase_if(names, [&](auto &key) { return !members.has(godot::String::utf8(key.c_str())); });
            return state->active() && godot::ObjectDB::get_instance(id) ? names : std::vector<std::string>{};
        };
    }
    auto query_keys = hooks.keys;
    auto query_has = hooks.has;
    auto proxy = runtime_->makeObject(id, std::move(hooks));
    if (shared_mode_ || enumerate_members_) {
        Root native_root(ctx, proxy);
        if (!object_factory_)
            object_factory_ =
                runtime_->evaluateRetained(R"JS((function(native,guard,keys,has){return new Proxy(Object.create(null),{
            get(t,k){guard();return Reflect.get(native,k);},
            set(t,k,v){guard();return Reflect.set(native,k,v);},
            has(t,k){guard();return typeof k==='string'&&has(k);},
            ownKeys(){guard();return keys();},
            getOwnPropertyDescriptor(t,k){guard();if(typeof k==='string'&&has(k))return {configurable:true,enumerable:keys().includes(k),writable:true,value:undefined};},
            deleteProperty(){guard();throw new TypeError('Object members cannot be deleted');},
            defineProperty(){guard();throw new TypeError('Object members cannot be redefined');},
            preventExtensions(){guard();throw new TypeError('Object proxy cannot be frozen');},
            setPrototypeOf(){guard();throw new TypeError('Object proxy prototype is fixed');}
        });}))JS");
        auto guard = runtime_->makeFunction(
            [weak = weak_from_this(), id](JSContextRef ctx, JSObjectRef, std::span<const JSValueRef>) -> JSValueRef {
                auto state = weak.lock();
                auto *source = godot::ObjectDB::get_instance(id);
                if (!state || !state->active() || !source)
                    throw std::runtime_error("Binding expired");
                if (source->has_method("_ultralight_alive") && !bool(source->call("_ultralight_alive")))
                    throw std::runtime_error("Managed Godot object was freed");
                return JSValueMakeUndefined(ctx);
            });
        Root guard_root(ctx, guard);
        auto keys =
            runtime_->makeFunction([weak = weak_from_this(), query_keys](JSContextRef ctx, JSObjectRef,
                                                                         std::span<const JSValueRef>) -> JSValueRef {
                auto state = weak.lock();
                if (!state || !state->active())
                    throw std::runtime_error("Binding expired");
                auto result = state->runtime_->makeArray();
                Root root(ctx, result);
                uint32_t i = 0;
                if (query_keys)
                    for (auto &name : query_keys(ctx))
                        state->runtime_->setAt(result, i++, state->runtime_->makeString(name));
                return result;
            });
        Root keys_root(ctx, keys);
        auto has = runtime_->makeFunction(
            [query_has](JSContextRef ctx, JSObjectRef, std::span<const JSValueRef> args) -> JSValueRef {
                auto name = JSValueToStringCopy(ctx, args[0], nullptr);
                bool found = query_has && query_has(ctx, name);
                JSStringRelease(name);
                return JSValueMakeBoolean(ctx, found);
            });
        Root has_root(ctx, has);
        JSValueRef args[] = {proxy, guard, keys, has}, error = nullptr;
        auto wrapped = JSObjectCallAsFunction(ctx, JSValueToObject(ctx, object_factory_->value(), nullptr), nullptr, 4,
                                              args, &error);
        check(error);
        proxy = JSValueToObject(ctx, wrapped, nullptr);
    }
    if (shared_mode_ || enumerate_members_) {
        objects_[id] = runtime_->retain(proxy);
        object_sources_[id] = object;
        host_sources_[proxy] = object;
    }
    return proxy;
}
} // namespace gdbind

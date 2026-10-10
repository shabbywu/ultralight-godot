#pragma once
#include "JavaScriptBridge__def.hpp"
#include "js_promise__def.hpp"
#include "js_string.hpp"
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace gdbind {
inline JSValueRef BridgeContext::writeObject(godot::Object *object) {
    auto ctx = runtime_->context();
    if (!object) return JSValueMakeNull(ctx);
    if (auto promise = godot::Object::cast_to<JavaScriptPromise>(object)) return promise->value(this);
    godot::ObjectID id(object->get_instance_id());
    return runtime_->makeObject(id,
        [weak = weak_from_this(), id](JSContextRef, JSStringRef name) -> JSValueRef {
            auto state = weak.lock();
            auto *instance = godot::ObjectDB::get_instance(id);
            if (!state || !state->active() || !instance)
                throw std::runtime_error("Godot object is no longer valid");
            auto property = godot_string(name);
            return state->toJS(instance->has_method(property)
                ? godot::Variant(godot::Callable(instance, property)) : instance->get(property));
        },
        [weak = weak_from_this(), id](JSContextRef, JSStringRef name, JSValueRef value) {
            auto state = weak.lock();
            auto *instance = godot::ObjectDB::get_instance(id);
            if (!state || !state->active() || !instance)
                throw std::runtime_error("Godot object is no longer valid");
            instance->set(godot_string(name), state->toGodot(value));
            return true;
        });
}
} // namespace gdbind

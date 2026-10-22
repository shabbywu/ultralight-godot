#pragma once
#include "JavaScriptBridge__def.hpp"
#include "js_callable_trampoline.hpp"
#include <godot_cpp/variant/callable.hpp>

namespace gdbind {
// Convert JavaScript functions to Godot Callables using the C# compatible
// trampoline. Identity is stable within each document and conversion mode.
inline godot::Variant BridgeContext::readFunction(JSObjectRef object, bool shared) {
    auto key = std::make_pair(object, shared);
    auto found = functions.find(key);
    if (found == functions.end()) {
        godot::Ref<JavascrtipCallableTrampoline> trampoline;
        trampoline.instantiate();
        trampoline->configure(weak_from_this(), runtime_->retain(object), shared);
        found = functions.emplace(key, trampoline).first;
    }
    return godot::Callable(found->second.ptr(), "trampoline");
}
} // namespace gdbind

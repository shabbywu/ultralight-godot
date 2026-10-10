#pragma once
#include "JavaScriptBridge__def.hpp"
#include "js_callable_trampoline.hpp"
#include <godot_cpp/variant/callable.hpp>

namespace gdbind {
// Convert JavaScript functions to Godot Callables using the C# compatible
// trampoline, reusing the same wrapper for each function in this document.
inline godot::Variant BridgeContext::readFunction(JSObjectRef object) {
    auto found = functions.find(object);
    if (found == functions.end()) {
        godot::Ref<JavascrtipCallableTrampoline> trampoline;
        trampoline.instantiate();
        trampoline->configure(weak_from_this(), object);
        runtime_->protect(object);
        found = functions.emplace(object, trampoline).first;
    }
    return godot::Callable(found->second.ptr(), "trampoline");
}
} // namespace gdbind

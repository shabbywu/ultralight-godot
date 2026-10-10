#pragma once
#include "js_callable_trampoline__def.hpp"
#include "JavaScriptBridge__def.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace gdbind {
inline void JavascrtipCallableTrampoline::configure(std::weak_ptr<BridgeContext> state, JSObjectRef fn) {
    context = state;
    function = fn;
}

inline void JavascrtipCallableTrampoline::_bind_methods() {
    godot::ClassDB::bind_vararg_method(godot::METHOD_FLAG_NORMAL, "trampoline",
        &JavascrtipCallableTrampoline::trampoline, godot::MethodInfo("trampoline"));
}

inline godot::Variant JavascrtipCallableTrampoline::trampoline(const godot::Variant **args, GDExtensionInt count,
                                                             GDExtensionCallError &error) {
    // C# can retain this RefCounted wrapper after its document expires. The
    // method still exists, but an expired callback must never enter the old VM.
    error.error = GDEXTENSION_CALL_OK;
    auto state = context.lock();
    if (!state || !state->active()) return godot::Variant();
    godot::Ref<JavascrtipCallableTrampoline> keep_alive(this);
    try {
        auto result = state->call(function, args, count);
        error.error = GDEXTENSION_CALL_OK;
        return result;
    } catch (const std::exception &exception) {
        godot::UtilityFunctions::push_error(exception.what());
    }
    return godot::Variant();
}

} // namespace gdbind

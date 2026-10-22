#pragma once
#include "JavaScriptBridge__def.hpp"
#include "js_callable_trampoline__def.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace gdbind {
inline void JavascrtipCallableTrampoline::configure(std::weak_ptr<BridgeContext> state,
                                                    std::unique_ptr<ulbind17::jsc::Bridge::Handle> fn, bool shared) {
    context = state;
    function = std::move(fn);
    this->shared = shared;
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
    if (!state || !state->active() || !function)
        return godot::Variant();
    godot::Ref<JavascrtipCallableTrampoline> keep_alive(this);
    try {
        auto result = state->call(*function, args, count, shared);
        error.error = GDEXTENSION_CALL_OK;
        return result;
    }
    catch (const std::exception &exception) {
        godot::UtilityFunctions::push_error(exception.what());
    }
    return godot::Variant();
}

} // namespace gdbind

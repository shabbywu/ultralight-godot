#pragma once
#include <ulbind17/jsc/Bridge.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <memory>

namespace gdbind {
class BridgeContext;

// A RefCounted trampoline keeps returned JavaScript functions callable from C#.
class JavascrtipCallableTrampoline : public godot::RefCounted {
    GDCLASS(JavascrtipCallableTrampoline, godot::RefCounted);
    std::weak_ptr<BridgeContext> context;
    std::unique_ptr<ulbind17::jsc::Bridge::Handle> function;
    godot::Variant trampoline(const godot::Variant **args, GDExtensionInt count, GDExtensionCallError &error);
  public:
    void configure(std::weak_ptr<BridgeContext> state, std::unique_ptr<ulbind17::jsc::Bridge::Handle> fn);
    static void _bind_methods();
};

} // namespace gdbind

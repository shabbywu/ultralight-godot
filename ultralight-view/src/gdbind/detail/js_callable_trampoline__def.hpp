#pragma once
#include <godot_cpp/classes/ref_counted.hpp>
#include <memory>
#include <ulbind17/jsc/Bridge.hpp>

namespace gdbind {
class BridgeContext;

// A RefCounted trampoline keeps returned JavaScript functions callable from C#.
class JavascrtipCallableTrampoline : public godot::RefCounted {
    GDCLASS(JavascrtipCallableTrampoline, godot::RefCounted);
    std::weak_ptr<BridgeContext> context;
    std::unique_ptr<ulbind17::jsc::Bridge::Handle> function;
    bool shared = false;
    godot::Variant trampoline(const godot::Variant **args, GDExtensionInt count, GDExtensionCallError &error);

  public:
    void configure(std::weak_ptr<BridgeContext> state, std::unique_ptr<ulbind17::jsc::Bridge::Handle> fn,
                   bool shared = false);
    static void _bind_methods();
};

} // namespace gdbind

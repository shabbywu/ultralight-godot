#pragma once
#include "js_callable.hpp"
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
using namespace godot;
namespace gdbind {
/// @brief godot-csharp does not support CallableCustom now, see also: https://github.com/godotengine/godot/issues/97358
///        So here convert an CallableCustom to Object method, wrap it as member method.
class JavascrtipCallableTrampoline : public Object {
    GDCLASS(JavascrtipCallableTrampoline, Object);

  public:
    JavascriptCallable *callable;

    virtual ~JavascrtipCallableTrampoline() {
        if (callable != nullptr)
            delete callable;
    }

  public:
    static void _bind_methods() {
        ClassDB::bind_vararg_method(METHOD_FLAG_NORMAL, "trampoline", &JavascrtipCallableTrampoline::trampoline,
                                    MethodInfo("trampoline"));
    }

  protected:
    Variant trampoline(const godot::Variant **p_arguments, GDExtensionInt p_argcount,
                       GDExtensionCallError &r_call_error) {
        godot::Variant out;
        callable->call(p_arguments, p_argcount, out, r_call_error);
        return out;
    }
};
} // namespace gdbind

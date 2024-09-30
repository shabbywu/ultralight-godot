#pragma once
#include "js_callable.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <map>
using namespace godot;
namespace gdbind {
/// @brief godot-csharp does not support CallableCustom now, see also: https://github.com/godotengine/godot/issues/97358
///        So here convert an CallableCustom to Object method, wrap it as member method.
class JavascrtipCallableTrampoline : public godot::RefCounted {
    GDCLASS(JavascrtipCallableTrampoline, godot::RefCounted);

  private:
    // TODO: find a better way to maintain refcounted
    static inline std::map<JSObjectRef, godot::Ref<JavascrtipCallableTrampoline>> instances;

  public:
    static void freeInstances() {
        instances.clear();
    }

    static JavascrtipCallableTrampoline* instantiate(ulbind17::detail::Object o) {
        if (auto it = instances.find(o.holder->value); it != instances.end()) {
            return it->second.ptr();
        }
        JavascrtipCallableTrampoline* out = memnew(gdbind::JavascrtipCallableTrampoline(new gdbind::JavascriptCallable(o)));
        instances.emplace(o.holder->value, out);
        return out;
    }

  public:
    JavascriptCallable *callable;
    JavascrtipCallableTrampoline() : RefCounted() {};
    JavascrtipCallableTrampoline(JavascriptCallable *callable) : RefCounted(), callable(callable) {
    }

    virtual ~JavascrtipCallableTrampoline() {
        if (callable != nullptr) {
            delete callable;
        }
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

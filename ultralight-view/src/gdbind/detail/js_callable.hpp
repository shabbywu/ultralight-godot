#pragma once

#include "cast__def.hpp"
#include <godot_cpp/variant/callable_custom.hpp>
#include <ulbind17/ulbind17.hpp>
namespace gdbind {
class JavascriptCallable : public godot::CallableCustom, public ulbind17::detail::Object {
  public:
    JavascriptCallable(ulbind17::detail::Object o) : Object(o.holder->ctx, o.holder->value) {
    }

    virtual ~JavascriptCallable() {
    }

    virtual uint32_t hash() const override {
        auto p = (intptr_t)holder->value;
        return p;
    };
    virtual godot::String get_as_text() const override {
        return godot::String(this->toString().data());
    }
    virtual CompareEqualFunc get_compare_equal_func() const override {
        return [](const godot::CallableCustom *a, const godot::CallableCustom *b) {
            return a->get_object() == b->get_object();
        };
    }
    virtual CompareLessFunc get_compare_less_func() const override {
        return [](const godot::CallableCustom *a, const godot::CallableCustom *b) {
            return a->get_object() < b->get_object();
        };
    }
    virtual bool is_valid() const override {
        return isFunction();
    }
    virtual godot::ObjectID get_object() const override {
        return godot::ObjectID();
    }
    virtual void call(const godot::Variant **p_arguments, int p_argcount, godot::Variant &r_return_value,
                      GDExtensionCallError &r_call_error) const override {
        JSValueRef *arguments = new JSValueRef[p_argcount];
        for (int i = 0; i < p_argcount; i++) {
            const godot::Variant &o = (*p_arguments[i]);
            arguments[i] = ulbind17::detail::generic_cast<const godot::Variant, JSValueRef>(holder->ctx, std::move(o));
        }
        JSValueRef exception = 0;
        JSValueRef result =
            JSObjectCallAsFunction(holder->ctx, holder->value, nullptr, p_argcount, arguments, &exception);
        if (arguments != nullptr)
            delete[] arguments;

        r_return_value = ulbind17::detail::generic_cast<JSValueRef, godot::Variant>(holder->ctx, std::move(result));
    };
};
} // namespace gdbind

#pragma once
#include "gdbind/api/Types.hpp"
#include <ulbind17/jsc/Bridge.hpp>
namespace gdbind {
class BridgeContext;
class UltralightJSValue : public godot::RefCounted {
    GDCLASS(UltralightJSValue, godot::RefCounted);

  protected:
    std::weak_ptr<BridgeContext> context_;
    std::unique_ptr<ulbind17::jsc::Bridge::Handle> handle_;
    std::shared_ptr<BridgeContext> context() const;
    static void _bind_methods();

  public:
    void configure(std::weak_ptr<BridgeContext> context, std::unique_ptr<ulbind17::jsc::Bridge::Handle> handle) {
        context_ = context;
        handle_ = std::move(handle);
    }
    JSValueRef value(const BridgeContext *expected) const;
    godot::Ref<UltralightResult> get_value(const godot::Variant &key);
    godot::Ref<UltralightResult> set_value(const godot::Variant &key, const godot::Variant &value);
    godot::Ref<UltralightResult> snapshot();
    godot::Ref<UltralightResult> utf8_bytes();
    godot::Ref<UltralightResult> keys();
    godot::Ref<UltralightResult> call(const godot::Array &arguments);
    godot::Ref<UltralightResult> invoke_method(const godot::String &name, const godot::Array &arguments);
    int64_t size();
    godot::Ref<UltralightResult> append(const godot::Variant &value);
    godot::Ref<UltralightResult> remove(const godot::Variant &key);
    void close() {
        handle_.reset();
    }
    bool is_closed() const;
};
class UltralightJSArray : public UltralightJSValue {
    GDCLASS(UltralightJSArray, UltralightJSValue);

  protected:
    static void _bind_methods() {
    }
};
class UltralightJSObject : public UltralightJSValue {
    GDCLASS(UltralightJSObject, UltralightJSValue);

  protected:
    static void _bind_methods() {
    }
};
class UltralightUndefined : public godot::RefCounted {
    GDCLASS(UltralightUndefined, godot::RefCounted);
    static godot::Ref<UltralightUndefined> &storage() {
        static godot::Ref<UltralightUndefined> instance;
        return instance;
    }

  protected:
    static void _bind_methods() {
    }

  public:
    static godot::Ref<UltralightUndefined> instance() {
        auto &value = storage();
        if (value.is_null())
            value.instantiate();
        return value;
    }
    static void shutdown() {
        storage().unref();
    }
};
} // namespace gdbind

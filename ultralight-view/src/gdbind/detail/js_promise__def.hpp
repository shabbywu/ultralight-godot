#pragma once
#include <ulbind17/jsc/Bridge.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <memory>

namespace gdbind {
class BridgeContext;

class JavaScriptPromise : public godot::RefCounted {
    GDCLASS(JavaScriptPromise, godot::RefCounted);
    std::weak_ptr<BridgeContext> context_;
    JSObjectRef promise_ = nullptr;
    std::unique_ptr<ulbind17::jsc::Bridge::Handle> handle_;
    bool completed_ = false, rejected_ = false;
    godot::Variant result_;
  public:
    void configure(std::weak_ptr<BridgeContext> context, JSObjectRef promise,
                   std::unique_ptr<ulbind17::jsc::Bridge::Handle> handle);
    bool is_completed() const { return completed_; }
    bool is_rejected() const { return rejected_; }
    godot::Variant get_result() const { return result_; }
    JSObjectRef value(const BridgeContext *context) const;
    void cancel();
    void settle(godot::Variant result, bool rejected);
    static void _bind_methods();
};

} // namespace gdbind

#pragma once
#include <ulbind17/jsc/Bridge.hpp>
#include <Ultralight/View.h>
#include "js_callable_trampoline__def.hpp"
#include "js_promise__def.hpp"
#include <godot_cpp/variant/variant.hpp>
#include <map>
#include <memory>
#include <vector>

namespace gdbind {
// Coordinates Godot type adapters for one document.
// ulbind17 owns the common JSC callbacks, roots, exceptions and document expiry.
class BridgeContext : public std::enable_shared_from_this<BridgeContext> {
    std::shared_ptr<ulbind17::jsc::Bridge> runtime_;
    ultralight::RefPtr<ultralight::View> owner;
    std::map<JSObjectRef, godot::Ref<JavascrtipCallableTrampoline>> functions;
    inline static std::vector<std::weak_ptr<BridgeContext>> contexts;
    inline static bool shutting_down = false;
    using Root = ulbind17::jsc::Root;

    void check(JSValueRef exception) const;
    godot::Variant read(JSValueRef value, std::vector<JSObjectRef> &parents);
    godot::Variant readFunction(JSObjectRef function);
    godot::Variant readPromise(JSObjectRef promise);
    godot::Variant rejectionReason(JSValueRef value);
    JSValueRef writeCallable(const godot::Callable &callable);
    JSValueRef writeObject(godot::Object *object);

  public:
    BridgeContext(JSContextRef context, ultralight::View *view);
    ~BridgeContext();
    static std::shared_ptr<BridgeContext> create(JSContextRef context, ultralight::View *view);
    bool active() const;
    bool matches(JSContextRef context) const;
    void invalidate() noexcept;
    static void shutdown();
    godot::Variant toGodot(JSValueRef value);
    JSValueRef toJS(const godot::Variant &value, unsigned depth = 0);
    void bind(const godot::String &name, const godot::Variant &value);
    godot::Variant evaluate(const godot::String &script);
    godot::Variant call(JSObjectRef function, const godot::Variant **args, int64_t count);
};

} // namespace gdbind

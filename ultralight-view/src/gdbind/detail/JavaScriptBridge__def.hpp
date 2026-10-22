#pragma once
#include "gdbind/api/Binding.hpp"
#include "gdbind/api/Types.hpp"
#include "js_callable_trampoline__def.hpp"
#include "js_promise__def.hpp"
#include "js_value__def.hpp"
#include <Ultralight/View.h>
#include <cstring>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <map>
#include <memory>
#include <optional>
#include <ulbind17/jsc/Bridge.hpp>
#include <unordered_map>
#include <vector>

namespace gdbind {
// Coordinates Godot type adapters for one document.
// ulbind17 owns the common JSC callbacks, roots, exceptions and document expiry.
class BridgeContext : public std::enable_shared_from_this<BridgeContext> {
    std::shared_ptr<ulbind17::jsc::Bridge> runtime_;
    ultralight::RefPtr<ultralight::View> owner;
    // COPY and SHARED calls need separate wrappers even for the same JS function.
    std::map<std::pair<JSObjectRef, bool>, godot::Ref<JavascrtipCallableTrampoline>> functions;
    inline static std::vector<std::weak_ptr<BridgeContext>> contexts;
    inline static bool shutting_down = false;
    struct Export {
        std::shared_ptr<BridgeContext> scope;
        std::unique_ptr<ulbind17::jsc::Bridge::Handle> target, value;
        std::string name;
    };
    friend class UltralightJSValue;
    bool invalidating_ = false;
    bool copy_own_keys_ = false;
    bool shared_mode_ = false, read_only_ = false, enumerate_members_ = false;
    std::map<godot::ObjectID, std::unique_ptr<ulbind17::jsc::Bridge::Handle>> objects_;
    std::map<godot::ObjectID, godot::Variant> object_sources_;
    struct HostContainer {
        godot::Variant source;
        std::unique_ptr<ulbind17::jsc::Bridge::Handle> proxy;
    };
    std::vector<HostContainer> containers_;
    std::map<std::pair<int, std::uintptr_t>, std::vector<size_t>> container_index_;
    std::map<JSObjectRef, godot::Variant> host_sources_;
    static std::optional<std::uintptr_t> containerIdentity(const godot::Variant &source);
    std::map<JSObjectRef, godot::Ref<UltralightJSValue>> js_values_;
    std::unique_ptr<ulbind17::jsc::Bridge::Handle> proxy_factory_, object_factory_, array_test_, object_tag_;
    JSValueRef writeContainer(const godot::Variant &source);
    godot::Variant readShared(JSValueRef value);
    godot::Variant copyHost(const godot::Variant &source, ulbind17::jsc::Traversal &traversal);
    bool isArray(JSValueRef value);
    void requireDataObject(JSObjectRef object);
    std::size_t arrayLength(JSValueRef value);
    std::map<std::string, Export> exports_;
    std::map<std::string, std::unique_ptr<ulbind17::jsc::Bridge::Handle>> namespaces_;
    struct AsyncDelivery {
        godot::Ref<UltralightTask> task;
        bool shared = true;
        std::unique_ptr<ulbind17::jsc::Bridge::Handle> resolve, reject;
    };
    std::vector<AsyncDelivery> deliveries_;
    godot::PackedStringArray allowed_members_;
    godot::ObjectID members_root_;
    using Root = ulbind17::jsc::Root;

    void check(JSValueRef exception) const;
    godot::Variant read(JSValueRef value, ulbind17::jsc::Traversal &traversal);
    godot::Variant readFunction(JSObjectRef function, bool shared = false);
    godot::Variant readPromise(JSObjectRef promise);
    godot::Variant rejectionReason(JSValueRef value);
    JSValueRef writeCallable(const godot::Callable &callable);
    JSValueRef writeObject(godot::Object *object, bool query_members = false);

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
    void ensureNamespace(const godot::String &name);
    void bindScoped(const godot::String &ns, const godot::String &name, const godot::Variant &value, bool async = false,
                    const godot::PackedStringArray &members = {}, bool copy = false, bool read_only = false);
    godot::Ref<UltralightBinding> bindTemporary(const godot::String &name, const godot::Variant &value,
                                                const godot::Ref<UltralightBindingOptions> &options);
    JSValueRef toSharedJS(const godot::Variant &value);
    void removeScoped(const godot::String &ns, const godot::String &name);
    void detachNamespace(const godot::String &ns);
    JSValueRef writeTask(const godot::Ref<UltralightTask> &task);
    JSValueRef writeAsync(const godot::Callable &handler);
    void evaluateAsync(const godot::String &source, const godot::Ref<UltralightTask> &task);
    static void pump();
    godot::Dictionary statistics() const;
    void bind(const godot::String &name, const godot::Variant &value);
    void bindObject(const godot::String &name, godot::Object *object, bool query_members);
    godot::Variant evaluate(const godot::String &script);
    godot::Variant snapshot(JSValueRef value);
    godot::Variant evaluateCopy(const godot::String &source);
    godot::Variant evaluateShared(const godot::String &script);
    godot::Ref<UltralightJSValue> evaluateHandle(const godot::String &script);
    godot::Variant call(const ulbind17::jsc::Bridge::Handle &function, const godot::Variant **args, int64_t count,
                        bool shared = false, const ulbind17::jsc::Bridge::Handle *receiver = nullptr);
};

} // namespace gdbind

#pragma once
#include "JavaScriptBridge__def.hpp"
#include "js_string.hpp"
#include <algorithm>
#include <utility>

namespace gdbind {
inline BridgeContext::BridgeContext(JSContextRef context, ultralight::View *view)
    : runtime_(ulbind17::jsc::Bridge::create(context, view)), owner(view) {}

inline std::shared_ptr<BridgeContext> BridgeContext::create(JSContextRef context, ultralight::View *view) {
    if (shutting_down) throw std::runtime_error("JavaScript bridge is shutting down");
    auto result = std::make_shared<BridgeContext>(context, view);
    std::erase_if(contexts, [](const auto &state) { return state.expired(); });
    contexts.push_back(result);
    return result;
}

inline BridgeContext::~BridgeContext() { invalidate(); }

inline bool BridgeContext::active() const { return runtime_->active(); }
inline bool BridgeContext::matches(JSContextRef context) const { return runtime_->matches(context); }

inline void BridgeContext::check(JSValueRef exception) const {
    ulbind17::jsc::check(runtime_->context(), exception);
}

inline void BridgeContext::invalidate() noexcept {
    // Cancellation signals can reset the host View and drop its last reference
    // to this context. Keep the context alive until its cleanup finishes.
    auto keep_alive = weak_from_this().lock();
    runtime_->invalidate();
    functions.clear();
    owner = nullptr;
}

inline void BridgeContext::shutdown() {
    // Cancelling a Promise emits Godot signals. Reentrant shutdown must not
    // iterate the registry again or allow a signal handler to create a bridge.
    shutting_down = true;
    auto pending = std::exchange(contexts, {});
    for (auto &weak : pending) if (auto context = weak.lock()) context->invalidate();
}

inline void BridgeContext::bind(const godot::String &name, const godot::Variant &value) {
    runtime_->set(JSContextGetGlobalObject(runtime_->context()), godot_utf8(name), toJS(value));
}

inline void BridgeContext::bindObject(const godot::String &name, godot::Object *object, bool query_members) {
    runtime_->set(JSContextGetGlobalObject(runtime_->context()), godot_utf8(name), writeObject(object, query_members));
}

inline godot::Variant BridgeContext::evaluate(const godot::String &script) {
    auto value = runtime_->evaluateRetained(godot_utf8(script));
    return toGodot(value->value());
}

inline godot::Variant BridgeContext::call(const ulbind17::jsc::Bridge::Handle &fn,
                                         const godot::Variant **args, int64_t count) {
    runtime_->context();
    if (!owner) throw std::runtime_error("JavaScript View is no longer valid");
    // Host callbacks may reset the View while this invocation is on the stack.
    auto view = owner;
    auto lock = view->LockJSContext();
    if (!matches(lock->ctx())) throw std::runtime_error("JavaScript function belongs to a previous document");
    std::vector<const ulbind17::jsc::Bridge::Handle *> values;
    std::vector<std::unique_ptr<ulbind17::jsc::Bridge::Handle>> handles;
    for (int64_t i = 0; i < count; ++i) {
        handles.push_back(runtime_->retain(toJS(*args[i])));
        values.push_back(handles.back().get());
    }
    auto value = runtime_->callRetained(fn, values);
    return toGodot(value->value());
}
} // namespace gdbind

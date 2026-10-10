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
    JSString key{name};
    runtime_->bind(key, toJS(value));
}

inline godot::Variant BridgeContext::evaluate(const godot::String &script) {
    JSString code{script};
    auto value = runtime_->evaluate(code);
    return value ? toGodot(value) : godot::Variant();
}

inline godot::Variant BridgeContext::call(JSObjectRef fn, const godot::Variant **args, int64_t count) {
    auto ctx = runtime_->context();
    if (!owner) throw std::runtime_error("JavaScript View is no longer valid");
    // Host callbacks may reset the View while this invocation is on the stack.
    auto view = owner;
    auto lock = view->LockJSContext();
    if (!matches(lock->ctx())) throw std::runtime_error("JavaScript function belongs to a previous document");
    std::vector<JSValueRef> values;
    std::vector<std::unique_ptr<Root>> roots;
    for (int64_t i = 0; i < count; ++i) {
        auto value = toJS(*args[i]);
        roots.push_back(std::make_unique<Root>(ctx, value));
        values.push_back(value);
    }
    auto value = runtime_->call(fn, values);
    return value ? toGodot(value) : godot::Variant();
}
} // namespace gdbind

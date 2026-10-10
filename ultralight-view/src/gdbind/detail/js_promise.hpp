#pragma once
#include "JavaScriptBridge__def.hpp"
#include "js_promise__def.hpp"
#include "js_string.hpp"
#include <godot_cpp/core/class_db.hpp>

namespace gdbind {
inline void JavaScriptPromise::configure(std::weak_ptr<BridgeContext> context, JSObjectRef promise,
                                         std::unique_ptr<ulbind17::jsc::Bridge::Handle> handle) {
    context_ = context;
    promise_ = promise;
    handle_ = std::move(handle);
}

inline JSObjectRef JavaScriptPromise::value(const BridgeContext *context) const {
    if (!handle_ || context_.lock().get() != context)
        throw std::runtime_error("Promise belongs to a different or expired document");
    handle_->value();
    return promise_;
}

inline void JavaScriptPromise::cancel() {
    handle_.reset();
    promise_ = nullptr;
    settle("JavaScript document expired", true);
}

inline void JavaScriptPromise::settle(godot::Variant result, bool rejected) {
    if (completed_) return;
    completed_ = true;
    rejected_ = rejected;
    result_ = std::move(result);
    emit_signal(rejected ? "rejected" : "resolved", result_);
    emit_signal("completed");
}

inline void JavaScriptPromise::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("is_completed"), &JavaScriptPromise::is_completed);
    godot::ClassDB::bind_method(godot::D_METHOD("is_rejected"), &JavaScriptPromise::is_rejected);
    godot::ClassDB::bind_method(godot::D_METHOD("get_result"), &JavaScriptPromise::get_result);
    ADD_SIGNAL(godot::MethodInfo("completed"));
    ADD_SIGNAL(godot::MethodInfo("resolved", godot::PropertyInfo(godot::Variant::NIL, "result",
        godot::PROPERTY_HINT_NONE, "", godot::PROPERTY_USAGE_NIL_IS_VARIANT)));
    ADD_SIGNAL(godot::MethodInfo("rejected", godot::PropertyInfo(godot::Variant::NIL, "reason",
        godot::PROPERTY_HINT_NONE, "", godot::PROPERTY_USAGE_NIL_IS_VARIANT)));
}

inline godot::Variant BridgeContext::rejectionReason(JSValueRef value) {
    auto ctx = runtime_->context();
    // Preserve ordinary rejection data. Error objects need an explicit text
    // conversion because their message/stack properties are non-enumerable.
    if (JSValueIsObject(ctx, value)) {
        JSString key{godot::String("Error")};
        JSValueRef exception = nullptr;
        auto constructor = JSObjectGetProperty(ctx, JSContextGetGlobalObject(ctx), key, &exception);
        check(exception);
        if (JSValueIsObject(ctx, constructor) &&
            JSValueIsInstanceOfConstructor(ctx, value, JSValueToObject(ctx, constructor, nullptr), &exception)) {
            check(exception);
            auto text = ulbind17::jsc::text(ctx, value);
            return godot::String::utf8(text.data(), text.size());
        }
        check(exception);
    }
    return toGodot(value);
}

inline godot::Variant BridgeContext::readPromise(JSObjectRef object) {
    godot::Ref<JavaScriptPromise> promise;
    promise.instantiate();
    promise->configure(weak_from_this(), object, runtime_->retain(object));
    runtime_->then(object, [weak = weak_from_this(), promise](JSValueRef result, bool rejected) {
        auto state = weak.lock();
        if (!state || !state->active()) {
            promise->settle("JavaScript document expired", true);
            return;
        }
        try {
            promise->settle(rejected ? state->rejectionReason(result) : state->toGodot(result), rejected);
        } catch (const std::exception &error) {
            promise->settle(godot::String(error.what()), true);
        }
    }, [promise] { promise->cancel(); });
    return promise;
}
} // namespace gdbind

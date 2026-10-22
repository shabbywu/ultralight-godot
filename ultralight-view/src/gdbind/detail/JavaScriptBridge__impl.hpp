#pragma once
#include "JavaScriptBridge__def.hpp"
#include "js_string.hpp"
#include <algorithm>
#include <utility>

namespace gdbind {
inline BridgeContext::BridgeContext(JSContextRef context, ultralight::View *view)
    : runtime_(ulbind17::jsc::Bridge::create(context, view)), owner(view) {
}

inline std::shared_ptr<BridgeContext> BridgeContext::create(JSContextRef context, ultralight::View *view) {
    if (shutting_down)
        throw std::runtime_error("JavaScript bridge is shutting down");
    auto result = std::make_shared<BridgeContext>(context, view);
    std::erase_if(contexts, [](const auto &state) { return state.expired(); });
    contexts.push_back(result);
    return result;
}

inline BridgeContext::~BridgeContext() {
    invalidate();
}

inline bool BridgeContext::active() const {
    return !invalidating_ && runtime_->active();
}
inline bool BridgeContext::matches(JSContextRef context) const {
    return active() && runtime_->matches(context);
}

inline void BridgeContext::check(JSValueRef exception) const {
    ulbind17::jsc::check(runtime_->context(), exception);
}

inline void BridgeContext::invalidate() noexcept {
    // Cancellation signals can reset the host View and drop its last reference
    // to this context. Keep the context alive until its cleanup finishes.
    if (invalidating_)
        return;
    invalidating_ = true;
    auto keep_alive = weak_from_this().lock();
    auto exports = std::exchange(exports_, {});
    for (auto &[key, item] : exports)
        item.scope->invalidate();
    auto deliveries = std::exchange(deliveries_, {});
    for (auto &item : deliveries) {
        // Revoking a binding can leave its document alive. Settle the JS
        // Promise before releasing its resolvers so callers do not hang.
        try {
            if (!shutting_down && runtime_->active()) {
                auto ctx = runtime_->context();
                auto message = runtime_->makeString("JavaScript binding or document expired");
                Root message_root(ctx, message);
                JSValueRef argument = JSObjectMakeError(ctx, 1, &message, nullptr);
                Root argument_root(ctx, argument);
                runtime_->set(argument, "code", runtime_->makeString("PageGone"));
                JSValueRef error = nullptr;
                JSObjectCallAsFunction(ctx, JSValueToObject(ctx, item.reject->value(), nullptr), nullptr, 1, &argument,
                                       &error);
            }
        }
        catch (...) {
            // Teardown can already have invalidated the parent document.
        }
        item.task->reject("PageGone", "JavaScript document expired");
    }
    namespaces_.clear();
    objects_.clear();
    object_sources_.clear();
    js_values_.clear();
    containers_.clear();
    container_index_.clear();
    host_sources_.clear();
    proxy_factory_.reset();
    object_factory_.reset();
    array_test_.reset();
    object_tag_.reset();
    runtime_->invalidate();
    functions.clear();
    owner = nullptr;
}

inline void BridgeContext::shutdown() {
    // Cancelling a Promise emits Godot signals. Reentrant shutdown must not
    // iterate the registry again or allow a signal handler to create a bridge.
    shutting_down = true;
    auto pending = std::exchange(contexts, {});
    for (auto &weak : pending)
        if (auto context = weak.lock())
            context->invalidate();
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

inline godot::Variant BridgeContext::call(const ulbind17::jsc::Bridge::Handle &fn, const godot::Variant **args,
                                          int64_t count, bool shared, const ulbind17::jsc::Bridge::Handle *receiver) {
    runtime_->context();
    if (!owner)
        throw std::runtime_error("JavaScript View is no longer valid");
    // Host callbacks may reset the View while this invocation is on the stack.
    auto view = owner;
    auto lock = view->LockJSContext();
    if (!matches(lock->ctx()))
        throw std::runtime_error("JavaScript function belongs to a previous document");
    std::vector<const ulbind17::jsc::Bridge::Handle *> values;
    std::vector<std::unique_ptr<ulbind17::jsc::Bridge::Handle>> handles;
    for (int64_t i = 0; i < count; ++i) {
        handles.push_back(runtime_->retain(shared ? toSharedJS(*args[i]) : toJS(*args[i])));
        values.push_back(handles.back().get());
    }
    auto value = runtime_->callRetained(fn, values, receiver);
    return shared ? readShared(value->value()) : toGodot(value->value());
}
inline void BridgeContext::ensureNamespace(const godot::String &name) {
    auto key = godot_utf8(name);
    auto ctx = runtime_->context();
    auto global = JSContextGetGlobalObject(ctx);
    if (auto existing = namespaces_.find(key); existing != namespaces_.end()) {
        if (!JSValueIsStrictEqual(ctx, runtime_->get(global, key), existing->second->value()))
            throw std::runtime_error("Namespace was replaced by page code: " + key);
        return;
    }
    if (runtime_->has(global, key))
        throw std::runtime_error("Namespace conflicts with existing global: " + key);
    auto object = runtime_->makePlainObject();
    auto handle = runtime_->retain(object);
    auto helpers = runtime_->makePlainObject();
    Root helper_root(ctx, helpers);
    runtime_->set(helpers, "copy",
                  runtime_->makeFunction([weak = weak_from_this()](JSContextRef, JSObjectRef,
                                                                   std::span<const JSValueRef> args) -> JSValueRef {
                      auto state = weak.lock();
                      if (!state || !state->active())
                          throw std::runtime_error("Document expired");
                      if (args.empty())
                          throw std::invalid_argument("copy requires a value");
                      auto snapshot = state->snapshot(args[0]);
                      return state->toJS(snapshot);
                  }));
    runtime_->set(helpers, "get",
                  runtime_->makeFunction([weak = weak_from_this()](JSContextRef, JSObjectRef,
                                                                   std::span<const JSValueRef> args) -> JSValueRef {
                      auto state = weak.lock();
                      if (!state || !state->active())
                          throw std::runtime_error("Document expired");
                      if (args.size() < 2)
                          throw std::invalid_argument("Expected dictionary and key");
                      auto source = state->readShared(args[0]);
                      if (source.get_type() != godot::Variant::DICTIONARY)
                          throw std::runtime_error("Expected host Dictionary");
                      godot::Dictionary dictionary = source;
                      auto key = state->readShared(args[1]);
                      return dictionary.has(key) ? state->toSharedJS(dictionary[key])
                                                 : state->runtime_->makeUndefined();
                  }));
    runtime_->set(helpers, "set",
                  runtime_->makeFunction([weak = weak_from_this()](JSContextRef, JSObjectRef,
                                                                   std::span<const JSValueRef> args) -> JSValueRef {
                      auto state = weak.lock();
                      if (!state || !state->active())
                          throw std::runtime_error("Document expired");
                      if (args.size() < 3)
                          throw std::invalid_argument("Expected dictionary, key and value");
                      for (auto &weak : contexts)
                          if (auto scope = weak.lock(); scope && scope->matches(state->runtime_->context()))
                              for (auto &host : scope->containers_)
                                  if (scope->read_only_ &&
                                      JSValueIsStrictEqual(scope->runtime_->context(), host.proxy->value(), args[0]))
                                      throw std::runtime_error("Binding is read-only");
                      auto source = state->readShared(args[0]);
                      if (source.get_type() != godot::Variant::DICTIONARY)
                          throw std::runtime_error("Expected host Dictionary");
                      godot::Dictionary dictionary = source;
                      if (dictionary.is_read_only())
                          throw std::runtime_error("Dictionary is read-only");
                      dictionary[state->readShared(args[1])] = state->readShared(args[2]);
                      return state->runtime_->makeUndefined();
                  }));
    runtime_->set(object, "$bridge", helpers);
    runtime_->set(global, key, object);
    namespaces_.emplace(key, std::move(handle));
}
inline void BridgeContext::bindScoped(const godot::String &ns, const godot::String &name, const godot::Variant &value,
                                      bool async, const godot::PackedStringArray &members, bool copy, bool read_only) {
    auto ctx = runtime_->context();
    if (!ns.is_empty())
        ensureNamespace(ns);
    auto target = ns.is_empty() ? JSContextGetGlobalObject(ctx)
                                : JSValueToObject(ctx, namespaces_.at(godot_utf8(ns))->value(), nullptr);
    auto child = create(ctx, owner.get());
    child->allowed_members_ = members;
    child->shared_mode_ = !copy;
    child->read_only_ = read_only;
    child->enumerate_members_ = true;
    if (value.get_type() == godot::Variant::OBJECT && static_cast<godot::Object *>(value))
        child->members_root_ = godot::ObjectID(static_cast<godot::Object *>(value)->get_instance_id());
    auto js = async ? child->writeAsync(godot::Callable(value)) : child->toJS(value);
    auto retained = runtime_->retain(js);
    auto target_handle = runtime_->retain(target);
    auto key = godot_utf8(ns) + "/" + godot_utf8(name);
    runtime_->set(target, godot_utf8(name), js);
    auto keep_alive = shared_from_this();
    std::shared_ptr<BridgeContext> previous;
    if (auto old = exports_.find(key); old != exports_.end())
        previous = old->second.scope;
    exports_.insert_or_assign(key, Export{child, std::move(target_handle), std::move(retained), godot_utf8(name)});
    if (previous)
        previous->invalidate();
}
inline void BridgeContext::removeScoped(const godot::String &ns, const godot::String &name) {
    auto keep_alive = shared_from_this();
    auto key = godot_utf8(ns) + "/" + godot_utf8(name);
    auto it = exports_.find(key);
    if (it == exports_.end())
        return;
    auto item = std::move(it->second);
    exports_.erase(it);
    auto ctx = runtime_->context();
    auto target = JSValueToObject(ctx, item.target->value(), nullptr);
    if (JSValueIsStrictEqual(ctx, runtime_->get(target, item.name), item.value->value())) {
        ulbind17::jsc::String property(item.name);
        JSValueRef error = nullptr;
        JSObjectDeleteProperty(ctx, target, property, &error);
        check(error);
    }
    item.scope->invalidate();
}
inline void BridgeContext::detachNamespace(const godot::String &ns) {
    auto keep_alive = shared_from_this();
    auto key = godot_utf8(ns);
    std::vector<std::string> names;
    for (auto &[k, item] : exports_)
        if (k.starts_with(key + "/"))
            names.push_back(item.name);
    auto it = namespaces_.find(key);
    if (it == namespaces_.end())
        return;
    auto ctx = runtime_->context();
    auto global = JSContextGetGlobalObject(ctx);
    if (JSValueIsStrictEqual(ctx, runtime_->get(global, key), it->second->value())) {
        ulbind17::jsc::String property(key);
        JSValueRef error = nullptr;
        JSObjectDeleteProperty(ctx, global, property, &error);
        check(error);
    }
    namespaces_.erase(it);
    for (auto &name : names) {
        if (!active())
            break;
        removeScoped(ns, godot::String::utf8(name.data(), name.size()));
    }
}
inline JSValueRef BridgeContext::writeTask(const godot::Ref<UltralightTask> &task) {
    auto ctx = runtime_->context();
    JSObjectRef resolve = nullptr, reject = nullptr;
    JSValueRef error = nullptr;
    auto promise = JSObjectMakeDeferredPromise(ctx, &resolve, &reject, &error);
    check(error);
    deliveries_.push_back({task, shared_mode_, runtime_->retain(resolve), runtime_->retain(reject)});
    return promise;
}
inline JSValueRef BridgeContext::writeAsync(const godot::Callable &handler) {
    return runtime_->makeFunction(
        [weak = weak_from_this(), handler](JSContextRef, JSObjectRef, std::span<const JSValueRef> args) -> JSValueRef {
            auto state = weak.lock();
            if (!state || !state->active())
                throw std::runtime_error("Binding expired");
            godot::Array values;
            for (auto arg : args)
                values.push_back(state->toGodot(arg));
            godot::Ref<UltralightTask> task;
            task.instantiate();
            auto promise = state->writeTask(task);
            Root keep(state->runtime_->context(), promise);
            handler.call(task, values);
            return promise;
        });
}
inline void BridgeContext::evaluateAsync(const godot::String &source, const godot::Ref<UltralightTask> &task) {
    auto value = runtime_->evaluateRetained(godot_utf8(source));
    if (!runtime_->isPromise(value->value())) {
        task->resolve(readShared(value->value()));
        return;
    }
    runtime_->then(
        value->value(),
        [weak = weak_from_this(), task](JSValueRef result, bool rejected) {
            if (task->is_completed())
                return;
            auto state = weak.lock();
            if (!state || !state->active()) {
                task->reject("PageGone", "Document expired");
                return;
            }
            try {
                if (rejected)
                    ulbind17::jsc::check(state->runtime_->context(), result);
                else
                    task->resolve(state->readShared(result));
            }
            catch (const std::exception &e) {
                task->finish(UltralightResult::failure(e));
            }
        },
        [task] { task->reject("PageGone", "Document expired"); });
}
inline void BridgeContext::pump() {
    ulbind17::jsc::Bridge::collectFinalizers();
    auto snapshot = contexts;
    for (auto &weak : snapshot)
        if (auto state = weak.lock(); state && state->active()) {
            auto pending = std::exchange(state->deliveries_, {});
            for (auto &item : pending) {
                if (!state->active()) {
                    item.task->reject("PageGone", "Document expired");
                    continue;
                }
                if (!item.task->is_completed()) {
                    state->deliveries_.push_back(std::move(item));
                    continue;
                }
                try {
                    auto ctx = state->runtime_->context();
                    auto result = item.task->get_result();
                    JSValueRef argument;
                    if (result->is_ok()) {
                        bool mode = std::exchange(state->shared_mode_, item.shared);
                        try {
                            argument = state->toJS(result->value);
                            state->shared_mode_ = mode;
                        }
                        catch (...) {
                            state->shared_mode_ = mode;
                            throw;
                        }
                    } else {
                        auto message = state->runtime_->makeString(godot_utf8(result->error->message));
                        Root message_root(ctx, message);
                        auto error = JSObjectMakeError(ctx, 1, &message, nullptr);
                        state->runtime_->set(error, "code",
                                             state->runtime_->makeString(godot_utf8(result->error->code)));
                        argument = error;
                    }
                    Root argument_root(ctx, argument);
                    auto &fn = result->is_ok() ? item.resolve : item.reject;
                    JSValueRef error = nullptr;
                    JSObjectCallAsFunction(ctx, JSValueToObject(ctx, fn->value(), nullptr), nullptr, 1, &argument,
                                           &error);
                    state->check(error);
                }
                catch (const std::exception &failure) {
                    if (state->active()) {
                        auto ctx = state->runtime_->context();
                        auto message = state->runtime_->makeString(failure.what());
                        Root keep(ctx, message);
                        JSValueRef error = nullptr;
                        JSObjectCallAsFunction(ctx, JSValueToObject(ctx, item.reject->value(), nullptr), nullptr, 1,
                                               &message, &error);
                    }
                }
            }
        }
}
inline godot::Ref<UltralightBinding> BridgeContext::bindTemporary(const godot::String &name,
                                                                  const godot::Variant &value,
                                                                  const godot::Ref<UltralightBindingOptions> &options) {
    bindScoped("", name, value, false, options.is_valid() ? options->members : godot::PackedStringArray(),
               options.is_valid() && options->copy_containers, options.is_valid() && options->read_only);
    auto key = "/" + godot_utf8(name);
    auto child = std::weak_ptr<BridgeContext>(exports_.at(key).scope);
    godot::Ref<UltralightBinding> result;
    result.instantiate();
    result->valid = [child] {
        auto state = child.lock();
        return state && state->active();
    };
    result->revoke = [weak = weak_from_this(), child, name, key] {
        auto root = weak.lock();
        auto scope = child.lock();
        if (!root || !root->active() || !scope)
            return;
        auto it = root->exports_.find(key);
        if (it != root->exports_.end() && it->second.scope == scope)
            root->removeScoped("", name);
    };
    return result;
}
inline JSValueRef BridgeContext::toSharedJS(const godot::Variant &value) {
    bool previous = std::exchange(shared_mode_, true);
    try {
        auto result = toJS(value);
        shared_mode_ = previous;
        return result;
    }
    catch (...) {
        shared_mode_ = previous;
        throw;
    }
}
inline godot::Dictionary BridgeContext::statistics() const {
    int64_t containers = containers_.size(), objects = objects_.size(), handles = 0, bindings = exports_.size();
    for (auto &[key, item] : js_values_)
        if (!item->is_closed())
            ++handles;
    for (auto &[key, item] : exports_) {
        auto nested = item.scope->statistics();
        containers += int64_t(nested["host_containers"]);
        objects += int64_t(nested["host_objects"]);
        handles += int64_t(nested["js_handles"]);
    }
    godot::Dictionary result;
    result["host_containers"] = containers;
    result["host_objects"] = objects;
    result["js_handles"] = handles;
    result["bindings"] = bindings;
    return result;
}
} // namespace gdbind

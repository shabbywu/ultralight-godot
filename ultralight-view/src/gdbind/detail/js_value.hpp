#pragma once
#include "JavaScriptBridge__def.hpp"
#include <godot_cpp/variant/utility_functions.hpp>
namespace gdbind {
inline bool BridgeContext::isArray(JSValueRef value) {
    auto ctx = runtime_->context();
    if (JSValueIsArray(ctx, value))
        return true;
    if (!JSValueIsObject(ctx, value))
        return false;
    if (!array_test_)
        array_test_ = runtime_->evaluateRetained("Array.isArray");
    JSValueRef error = nullptr;
    auto result =
        JSObjectCallAsFunction(ctx, JSValueToObject(ctx, array_test_->value(), nullptr), nullptr, 1, &value, &error);
    check(error);
    return JSValueToBoolean(ctx, result);
}
inline std::size_t BridgeContext::arrayLength(JSValueRef value) {
    auto size = runtime_->readNumber(runtime_->get(value, "length"));
    if (!std::isfinite(size) || size < 0 || size > 4294967295.0 || std::floor(size) != size)
        throw std::runtime_error("Invalid array length");
    return std::size_t(size);
}
inline void BridgeContext::requireDataObject(JSObjectRef object) {
    if (!object_tag_)
        object_tag_ = runtime_->evaluateRetained("Object.prototype.toString");
    auto ctx = runtime_->context();
    JSValueRef error = nullptr;
    auto tag =
        JSObjectCallAsFunction(ctx, JSValueToObject(ctx, object_tag_->value(), nullptr), object, 0, nullptr, &error);
    check(error);
    auto name = runtime_->readString(tag);
    if (name != "[object Object]")
        throw ulbind17::Error(ulbind17::ErrorCode::Unsupported, "Unsupported JavaScript data type " + name +
                                                                    "; project its data explicitly or retain a handle");
}
inline godot::Variant BridgeContext::copyHost(const godot::Variant &source, ulbind17::jsc::Traversal &traversal) {
    using V = godot::Variant;
    if (source.get_type() == V::OBJECT) {
        godot::Object *object = source;
        if (!object)
            return godot::Variant();
        if (auto live = godot::Object::cast_to<UltralightJSValue>(object))
            return read(live->value(this), traversal);
        if (object->has_method("_ultralight_kind")) {
            auto guard = traversal.enter(reinterpret_cast<void *>(object->get_instance_id()));
            auto kind = godot::String(object->call("_ultralight_kind"));
            if (kind == "array") {
                godot::Array result;
                int64_t size = managedCall(object, "_ultralight_get_result", godot::String("length"));
                result.resize(size);
                for (int64_t i = 0; i < size; ++i)
                    result[i] =
                        copyHost(managedCall(object, "_ultralight_get_result", godot::String::num_int64(i)), traversal);
                return result;
            }
            godot::Dictionary result;
            godot::Array keys = managedCall(object, "_ultralight_keys_result");
            for (int64_t i = 0; i < keys.size(); ++i)
                result[keys[i]] =
                    copyHost(managedCall(object, "_ultralight_get_result", godot::String(keys[i])), traversal);
            return result;
        }
        return read(toSharedJS(source), traversal);
    }
    if (source.get_type() == V::CALLABLE)
        throw std::runtime_error("Functions cannot be copied; retain a handle");
    if (source.get_type() != V::ARRAY && source.get_type() != V::DICTIONARY)
        return source;
    auto token = containerIdentity(source).value_or(reinterpret_cast<std::uintptr_t>(&source));
    auto guard = traversal.enter(reinterpret_cast<void *>(token));
    if (source.get_type() == V::ARRAY) {
        godot::Array values = source, result;
        result.resize(values.size());
        for (int64_t i = 0; i < values.size(); ++i)
            result[i] = copyHost(values[i], traversal);
        return result;
    }
    godot::Dictionary values = source, result;
    auto keys = values.keys();
    for (int64_t i = 0; i < keys.size(); ++i)
        if (keys[i].get_type() == V::STRING || keys[i].get_type() == V::STRING_NAME)
            result[godot::String(keys[i])] = copyHost(values[keys[i]], traversal);
    return result;
}
inline godot::Variant BridgeContext::readShared(JSValueRef value) {
    auto ctx = runtime_->context();
    if (!JSValueIsObject(ctx, value)) {
        bool previous = std::exchange(shared_mode_, true);
        try {
            ulbind17::jsc::Traversal t;
            auto result = read(value, t);
            shared_mode_ = previous;
            return result;
        }
        catch (...) {
            shared_mode_ = previous;
            throw;
        }
    }
    auto object = JSValueToObject(ctx, value, nullptr);
    // A proxy passed across binding scopes still denotes its original source.
    for (auto &weak : contexts)
        if (auto scope = weak.lock(); scope && scope->matches(ctx)) {
            if (auto id = scope->runtime_->unwrap<godot::ObjectID>(value))
                return godot::ObjectDB::get_instance(*id);
            if (auto found = scope->host_sources_.find(object); found != scope->host_sources_.end())
                return found->second;
        }
    if (runtime_->isPromise(value))
        return readPromise(object);
    if (JSObjectIsFunction(ctx, object))
        return readFunction(object, true);
    if (auto it = js_values_.find(object); it != js_values_.end()) {
        if (!it->second->is_closed())
            return it->second;
        js_values_.erase(it);
    }
    godot::Ref<UltralightJSValue> result;
    if (isArray(value)) {
        godot::Ref<UltralightJSArray> a;
        a.instantiate();
        result = a;
    } else {
        requireDataObject(object);
        godot::Ref<UltralightJSObject> o;
        o.instantiate();
        result = o;
    }
    result->configure(weak_from_this(), runtime_->retain(value));
    js_values_.emplace(object, result);
    return result;
}
inline godot::Variant BridgeContext::snapshot(JSValueRef value) {
    bool previous = std::exchange(shared_mode_, true), own = std::exchange(copy_own_keys_, true);
    try {
        ulbind17::jsc::Traversal traversal;
        auto result = read(value, traversal);
        shared_mode_ = previous;
        copy_own_keys_ = own;
        return result;
    }
    catch (...) {
        shared_mode_ = previous;
        copy_own_keys_ = own;
        throw;
    }
}
inline godot::Variant BridgeContext::evaluateCopy(const godot::String &source) {
    auto value = runtime_->evaluateRetained(godot_utf8(source));
    return snapshot(value->value());
}
inline godot::Variant BridgeContext::evaluateShared(const godot::String &source) {
    auto value = runtime_->evaluateRetained(godot_utf8(source));
    return readShared(value->value());
}
inline godot::Ref<UltralightJSValue> BridgeContext::evaluateHandle(const godot::String &source) {
    godot::Ref<UltralightJSValue> result;
    result.instantiate();
    result->configure(weak_from_this(), runtime_->evaluateRetained(godot_utf8(source)));
    return result;
}
// The pinned 4.3 ABI stores Array/Dictionary as opaque reference handles. Copy
// their bytes as a hash hint only: never dereference an engine-private pointer.
// Verify alias copies on startup and every cache hit with Godot's is_same.
// Unknown wrapper layouts fall back to the public reference-equality predicate.
inline std::optional<std::uintptr_t> BridgeContext::containerIdentity(const godot::Variant &source) {
    static const bool supported = [] {
        if (sizeof(godot::Array) != sizeof(std::uintptr_t) || sizeof(godot::Dictionary) != sizeof(std::uintptr_t))
            return false;
        godot::Array a, b = a;
        godot::Dictionary d, e = d;
        return std::memcmp(a._native_ptr(), b._native_ptr(), sizeof(std::uintptr_t)) == 0 &&
               std::memcmp(d._native_ptr(), e._native_ptr(), sizeof(std::uintptr_t)) == 0;
    }();
    if (source.get_type() == godot::Variant::OBJECT) {
        godot::Object *object = source;
        return object ? std::optional<std::uintptr_t>(object->get_instance_id()) : std::nullopt;
    }
    if (!supported)
        return std::nullopt;
    std::uintptr_t token = 0;
    if (source.get_type() == godot::Variant::ARRAY) {
        godot::Array value = source;
        std::memcpy(&token, value._native_ptr(), sizeof(token));
    } else {
        godot::Dictionary value = source;
        std::memcpy(&token, value._native_ptr(), sizeof(token));
    }
    return token;
}
inline JSValueRef BridgeContext::writeContainer(const godot::Variant &source) {
    auto ctx = runtime_->context();
    auto identity = containerIdentity(source);
    auto key = std::make_pair(int(source.get_type()), identity.value_or(0));
    if (identity) {
        if (auto found = container_index_.find(key); found != container_index_.end())
            for (auto index : found->second)
                if (godot::UtilityFunctions::is_same(containers_[index].source, source))
                    return containers_[index].proxy->value();
    } else
        for (auto &item : containers_)
            if (godot::UtilityFunctions::is_same(item.source, source))
                return item.proxy->value();
    if (!proxy_factory_)
        proxy_factory_ = runtime_->evaluateRetained(R"JS((function(array,get,set,del,keys,has){
      const target=array?[]:Object.create(null);
      const index=k=>typeof k==='string' && /^(0|[1-9][0-9]*)$/.test(k) && Number(k)<4294967295;
      const sync=()=>{if(array)target.length=get('length');};
      return new Proxy(target,{
        get(t,k,r){
          if(typeof k==='symbol'){get('length');return Reflect.get(t,k,r);}
          if(array && k!=='length' && !index(k)){get('length');return Reflect.get(t,k,r);}
          return get(k);
        },
        set(t,k,v){if(typeof k!=='string' || (array && k!=='length' && !index(k)))throw new TypeError('Unsupported property');return set(k,v);},
        deleteProperty(t,k){if(typeof k!=='string' || (array && !index(k)))throw new TypeError('Unsupported deletion');return del(k);},
        has(t,k){get('length');return typeof k==='string' && has(k) || Reflect.has(t,k);},
        ownKeys(){sync();const result=keys();return array?result.concat('length'):result;},
        getOwnPropertyDescriptor(t,k){sync();if(array&&k==='length')return Reflect.getOwnPropertyDescriptor(t,k);
          if(typeof k==='string'&&has(k))return {enumerable:true,configurable:true,writable:true,value:undefined};},
        defineProperty(){throw new TypeError('Shared containers do not support defineProperty');},
        preventExtensions(){throw new TypeError('Shared containers cannot be frozen');},
        setPrototypeOf(){throw new TypeError('Shared container prototype is fixed');}
      });
    }))JS");
    godot::Object *managed =
        source.get_type() == godot::Variant::OBJECT ? static_cast<godot::Object *>(source) : nullptr;
    bool array = source.get_type() == godot::Variant::ARRAY ||
                 (managed && godot::String(managed->call("_ultralight_kind")) == "array");
    auto access = [weak = weak_from_this(), source, array](std::string key) -> godot::Variant {
        auto state = weak.lock();
        if (!state || !state->active())
            throw std::runtime_error("Binding expired");
        if (source.get_type() == godot::Variant::OBJECT) {
            godot::Object *object = source;
            auto name = godot::String::utf8(key.data(), key.size());
            if (!bool(object->call("_ultralight_has", name)))
                return UltralightUndefined::instance();
            return managedCall(object, "_ultralight_get_result", name);
        }
        if (array) {
            godot::Array a = source;
            if (key == "length")
                return a.size();
            try {
                size_t end;
                auto i = std::stoull(key, &end);
                if (end == key.size() && i < uint64_t(a.size()))
                    return a[i];
            }
            catch (...) {
            }
            return UltralightUndefined::instance();
        }
        godot::Dictionary d = source;
        auto name = godot::String::utf8(key.data(), key.size());
        if (d.has(name))
            return d[name];
        if (d.has(godot::StringName(name)))
            return d[godot::StringName(name)];
        return UltralightUndefined::instance();
    };
    auto get =
        runtime_->makeFunction([weak = weak_from_this(), access](JSContextRef ctx, JSObjectRef,
                                                                 std::span<const JSValueRef> args) -> JSValueRef {
            auto state = weak.lock();
            if (!state || !state->active())
                throw std::runtime_error("Binding expired");
            return state->toSharedJS(access(ulbind17::jsc::text(ctx, args[0])));
        });
    Root get_root(ctx, get);
    auto set =
        runtime_->makeFunction([weak = weak_from_this(), source,
                                array](JSContextRef ctx, JSObjectRef, std::span<const JSValueRef> args) -> JSValueRef {
            auto state = weak.lock();
            if (!state || !state->active())
                throw std::runtime_error("Binding expired");
            if (state->read_only_)
                throw std::runtime_error("Binding is read-only");
            auto key = ulbind17::jsc::text(ctx, args[0]);
            auto value = state->readShared(args[1]);
            if (source.get_type() == godot::Variant::OBJECT) {
                godot::Object *object = source;
                managedCall(object, "_ultralight_set_result", godot::String::utf8(key.data(), key.size()), value);
                return JSValueMakeBoolean(ctx, true);
            }
            if (array) {
                godot::Array a = source;
                if (a.is_read_only())
                    throw std::runtime_error("Array is read-only");
                if (key == "length") {
                    auto length = state->runtime_->readNumber(args[1]);
                    if (!std::isfinite(length) || length < 0 || length > 10000000 || std::floor(length) != length)
                        throw std::runtime_error("Invalid array length");
                    auto old = a.size();
                    if (a.is_typed() && length > old)
                        throw std::runtime_error("Cannot fill a typed array with undefined");
                    if (a.resize(length) != godot::OK)
                        throw std::runtime_error("Array resize failed");
                    for (int64_t i = old; i < a.size(); ++i)
                        a[i] = UltralightUndefined::instance();
                } else {
                    auto index = std::stoull(key);
                    if (index > 10000000)
                        throw std::runtime_error("Array index too large");
                    if (a.is_typed() && int(value.get_type()) != a.get_typed_builtin())
                        throw std::runtime_error("Typed array element mismatch");
                    if (index >= uint64_t(a.size())) {
                        if (a.is_typed() && index != uint64_t(a.size()))
                            throw std::runtime_error("Typed array cannot contain undefined gaps");
                        auto old = a.size();
                        if (a.resize(index + 1) != godot::OK)
                            throw std::runtime_error("Array resize failed");
                        for (int64_t i = old; i < int64_t(index); ++i)
                            a[i] = UltralightUndefined::instance();
                    }
                    a[index] = value;
                }
            } else {
                godot::Dictionary d = source;
                if (d.is_read_only())
                    throw std::runtime_error("Dictionary is read-only");
                auto name = godot::String::utf8(key.data(), key.size());
                if (d.has(godot::StringName(name)) && !d.has(name))
                    d[godot::StringName(name)] = value;
                else
                    d[name] = value;
            }
            return JSValueMakeBoolean(ctx, true);
        });
    Root set_root(ctx, set);
    auto del =
        runtime_->makeFunction([weak = weak_from_this(), source,
                                array](JSContextRef ctx, JSObjectRef, std::span<const JSValueRef> args) -> JSValueRef {
            auto state = weak.lock();
            if (!state || !state->active())
                throw std::runtime_error("Binding expired");
            if (state->read_only_)
                throw std::runtime_error("Binding is read-only");
            auto key = ulbind17::jsc::text(ctx, args[0]);
            if (source.get_type() == godot::Variant::OBJECT) {
                godot::Object *object = source;
                managedCall(object, "_ultralight_delete_result", godot::String::utf8(key.data(), key.size()));
                return JSValueMakeBoolean(ctx, true);
            }
            if (array) {
                godot::Array a = source;
                if (a.is_read_only() || a.is_typed())
                    throw std::runtime_error("Cannot delete this array element");
                auto i = std::stoull(key);
                if (i < uint64_t(a.size()))
                    a[i] = UltralightUndefined::instance();
            } else {
                godot::Dictionary d = source;
                if (d.is_read_only())
                    throw std::runtime_error("Dictionary is read-only");
                auto name = godot::String::utf8(key.data(), key.size());
                d.erase(name);
                d.erase(godot::StringName(name));
            }
            return JSValueMakeBoolean(ctx, true);
        });
    Root del_root(ctx, del);
    auto keys = runtime_->makeFunction([weak = weak_from_this(), source, array](
                                           JSContextRef ctx, JSObjectRef, std::span<const JSValueRef>) -> JSValueRef {
        auto state = weak.lock();
        if (!state || !state->active())
            throw std::runtime_error("Binding expired");
        auto result = state->runtime_->makeArray();
        Root root(ctx, result);
        if (source.get_type() == godot::Variant::OBJECT) {
            godot::Object *object = source;
            godot::Array names = managedCall(object, "_ultralight_keys_result");
            for (int64_t i = 0; i < names.size(); ++i)
                state->runtime_->setAt(result, i, state->runtime_->makeString(godot_utf8(godot::String(names[i]))));
        } else if (array) {
            godot::Array a = source;
            for (int64_t i = 0; i < a.size(); ++i)
                state->runtime_->setAt(result, i, state->runtime_->makeString(std::to_string(i)));
        } else {
            godot::Dictionary d = source;
            auto names = d.keys();
            uint32_t i = 0;
            for (int64_t n = 0; n < names.size(); ++n) {
                if (names[n].get_type() == godot::Variant::STRING || names[n].get_type() == godot::Variant::STRING_NAME)
                    state->runtime_->setAt(result, i++,
                                           state->runtime_->makeString(godot_utf8(godot::String(names[n]))));
            }
        }
        return result;
    });
    Root keys_root(ctx, keys);
    auto has =
        runtime_->makeFunction([weak = weak_from_this(), source,
                                array](JSContextRef ctx, JSObjectRef, std::span<const JSValueRef> args) -> JSValueRef {
            auto state = weak.lock();
            if (!state || !state->active())
                throw std::runtime_error("Binding expired");
            auto key = ulbind17::jsc::text(ctx, args[0]);
            bool found = false;
            if (source.get_type() == godot::Variant::OBJECT) {
                godot::Object *object = source;
                found = object->call("_ultralight_has", godot::String::utf8(key.data(), key.size()));
            } else if (array) {
                godot::Array a = source;
                if (key == "length")
                    found = true;
                else
                    try {
                        size_t end;
                        auto i = std::stoull(key, &end);
                        found = end == key.size() && i < uint64_t(a.size());
                    }
                    catch (...) {
                    }
            } else {
                godot::Dictionary d = source;
                auto name = godot::String::utf8(key.data(), key.size());
                found = d.has(name) || d.has(godot::StringName(name));
            }
            return JSValueMakeBoolean(ctx, found);
        });
    Root has_root(ctx, has);
    JSValueRef args[] = {JSValueMakeBoolean(ctx, array), get, set, del, keys, has};
    JSValueRef error = nullptr;
    auto proxy =
        JSObjectCallAsFunction(ctx, JSValueToObject(ctx, proxy_factory_->value(), nullptr), nullptr, 6, args, &error);
    check(error);
    if (identity)
        container_index_[key].push_back(containers_.size());
    containers_.push_back({source, runtime_->retain(proxy)});
    host_sources_[JSValueToObject(ctx, proxy, nullptr)] = source;
    return proxy;
}
inline std::shared_ptr<BridgeContext> UltralightJSValue::context() const {
    auto result = context_.lock();
    if (!handle_ || !result || !result->active())
        throw ulbind17::Error(ulbind17::ErrorCode::PageGone, "JavaScript value expired");
    return result;
}
inline JSValueRef UltralightJSValue::value(const BridgeContext *expected) const {
    auto state = context();
    if (state.get() != expected && !expected->matches(state->runtime_->context()))
        throw ulbind17::Error(ulbind17::ErrorCode::CrossContext, "JavaScript value belongs to another document");
    return handle_->value();
}
inline bool UltralightJSValue::is_closed() const {
    auto state = context_.lock();
    return !handle_ || !state || !state->active();
}
inline godot::Ref<UltralightResult> UltralightJSValue::get_value(const godot::Variant &key) {
    try {
        auto state = context();
        return UltralightResult::success(
            state->readShared(state->runtime_->get(handle_->value(), godot_utf8(godot::String(key)))));
    }
    catch (const std::exception &e) {
        return UltralightResult::failure(e);
    }
}
inline godot::Ref<UltralightResult> UltralightJSValue::set_value(const godot::Variant &key,
                                                                 const godot::Variant &value) {
    try {
        auto state = context();
        bool previous = std::exchange(state->shared_mode_, true);
        try {
            state->runtime_->set(handle_->value(), godot_utf8(godot::String(key)), state->toJS(value));
            state->shared_mode_ = previous;
        }
        catch (...) {
            state->shared_mode_ = previous;
            throw;
        }
        return UltralightResult::success();
    }
    catch (const std::exception &e) {
        return UltralightResult::failure(e);
    }
}
inline godot::Ref<UltralightResult> UltralightJSValue::snapshot() {
    try {
        auto state = context();
        return UltralightResult::success(state->snapshot(handle_->value()));
    }
    catch (const std::exception &e) {
        return UltralightResult::failure(e);
    }
}
inline godot::Ref<UltralightResult> UltralightJSValue::keys() {
    try {
        auto state = context();
        godot::Array names;
        for (auto &name : state->runtime_->keys(handle_->value()))
            names.push_back(godot::String::utf8(name.data(), name.size()));
        return UltralightResult::success(names);
    }
    catch (const std::exception &e) {
        return UltralightResult::failure(e);
    }
}
inline int64_t UltralightJSValue::size() {
    try {
        auto state = context();
        return state->isArray(handle_->value()) ? state->arrayLength(handle_->value())
                                                : state->runtime_->keys(handle_->value()).size();
    }
    catch (...) {
        return -1;
    }
}
inline godot::Ref<UltralightResult> UltralightJSValue::append(const godot::Variant &value) {
    try {
        auto state = context();
        if (!state->isArray(handle_->value()))
            return UltralightResult::failure("TypeMismatch", "append requires a JavaScript Array");
        return set_value(int64_t(state->arrayLength(handle_->value())), value);
    }
    catch (const std::exception &e) {
        return UltralightResult::failure(e);
    }
}
inline godot::Ref<UltralightResult> UltralightJSValue::remove(const godot::Variant &key) {
    try {
        auto state = context();
        auto ctx = state->runtime_->context();
        JSValueRef error = nullptr;
        ulbind17::jsc::String name(godot_utf8(godot::String(key)));
        bool removed = JSObjectDeleteProperty(ctx, JSValueToObject(ctx, handle_->value(), nullptr), name, &error);
        state->check(error);
        if (!removed)
            return UltralightResult::failure("TypeMismatch", "JavaScript property cannot be deleted");
        return UltralightResult::success();
    }
    catch (const std::exception &e) {
        return UltralightResult::failure(e);
    }
}
inline godot::Ref<UltralightResult> UltralightJSValue::call(const godot::Array &arguments) {
    try {
        auto state = context();
        std::vector<godot::Variant> values;
        for (int64_t i = 0; i < arguments.size(); ++i)
            values.push_back(arguments[i]);
        std::vector<const godot::Variant *> ptrs;
        for (auto &v : values)
            ptrs.push_back(&v);
        return UltralightResult::success(state->call(*handle_, ptrs.data(), ptrs.size(), true));
    }
    catch (const std::exception &e) {
        return UltralightResult::failure(e);
    }
}
inline godot::Ref<UltralightResult> UltralightJSValue::invoke_method(const godot::String &name,
                                                                     const godot::Array &arguments) {
    try {
        auto state = context();
        auto function = state->runtime_->getRetained(*handle_, godot_utf8(name));
        std::vector<godot::Variant> values;
        for (int64_t i = 0; i < arguments.size(); ++i)
            values.push_back(arguments[i]);
        std::vector<const godot::Variant *> pointers;
        for (auto &value : values)
            pointers.push_back(&value);
        // The checked call validates callability and keeps this receiver alive.
        return UltralightResult::success(state->call(*function, pointers.data(), pointers.size(), true, handle_.get()));
    }
    catch (const std::exception &e) {
        return UltralightResult::failure(e);
    }
}
inline godot::Ref<UltralightResult> UltralightJSValue::utf8_bytes() {
    try {
        auto state = context();
        auto text = ulbind17::jsc::text(state->runtime_->context(), handle_->value());
        godot::PackedByteArray bytes;
        bytes.resize(text.size());
        if (!text.empty())
            std::memcpy(bytes.ptrw(), text.data(), text.size());
        return UltralightResult::success(bytes);
    }
    catch (const std::exception &e) {
        return UltralightResult::failure(e);
    }
}
inline void UltralightJSValue::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("get_value", "key"), &UltralightJSValue::get_value);
    ClassDB::bind_method(D_METHOD("set_value", "key", "value"), &UltralightJSValue::set_value);
    ClassDB::bind_method(D_METHOD("utf8_bytes"), &UltralightJSValue::utf8_bytes);
    ClassDB::bind_method(D_METHOD("snapshot"), &UltralightJSValue::snapshot);
    ClassDB::bind_method(D_METHOD("keys"), &UltralightJSValue::keys);
    ClassDB::bind_method(D_METHOD("size"), &UltralightJSValue::size);
    ClassDB::bind_method(D_METHOD("append", "value"), &UltralightJSValue::append);
    ClassDB::bind_method(D_METHOD("remove", "key"), &UltralightJSValue::remove);
    ClassDB::bind_method(D_METHOD("invoke_method", "name", "arguments"), &UltralightJSValue::invoke_method);
    ClassDB::bind_method(D_METHOD("invoke", "arguments"), &UltralightJSValue::call);
    ClassDB::bind_method(D_METHOD("close"), &UltralightJSValue::close);
    ClassDB::bind_method(D_METHOD("is_closed"), &UltralightJSValue::is_closed);
}
} // namespace gdbind

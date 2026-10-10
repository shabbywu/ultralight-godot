#pragma once
#include "JavaScriptBridge__def.hpp"
#include <godot_cpp/variant/callable.hpp>
#include <string>
#include <vector>

namespace gdbind {
inline JSValueRef BridgeContext::writeCallable(const godot::Callable &callable) {
    return runtime_->makeFunction([weak = weak_from_this(), callable](
        JSContextRef context, JSObjectRef, std::span<const JSValueRef> args) -> JSValueRef {
        auto state = weak.lock();
        if (!state || !state->active() || !callable.is_valid())
            throw std::runtime_error("Godot callback is no longer valid");
        std::vector<godot::Variant> values;
        values.reserve(args.size());
        for (auto arg : args) values.push_back(state->toGodot(arg));
        std::vector<const godot::Variant *> arguments;
        arguments.reserve(values.size());
        for (const auto &value : values) arguments.push_back(&value);
        godot::Variant target(callable), result;
        GDExtensionCallError error{};
        target.callp("call", arguments.data(), static_cast<int>(arguments.size()), result, error);
        if (error.error != GDEXTENSION_CALL_OK)
            throw std::runtime_error("Godot callback failed (error " + std::to_string(error.error) +
                                     ", argument " + std::to_string(error.argument) +
                                     ", expected " + std::to_string(error.expected) + ")");
        return state->active() ? state->toJS(result) : JSValueMakeUndefined(context);
    });
}
} // namespace gdbind

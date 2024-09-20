#pragma once
#include <Ultralight/Ultralight.h>
#include <godot_cpp/variant/utility_functions.hpp>
namespace gdbind {
struct ViewListener : public ultralight::ViewListener {
    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
    };
    LogLevel logLevel = LogLevel::INFO;

    ///
    /// Called when a message is added to the console (useful for errors / debug)
    ///
    virtual void OnAddConsoleMessage(ultralight::View *caller, const ultralight::ConsoleMessage &message) {
        auto level = message.level();
        if (level == ultralight::MessageLevel::kMessageLevel_Error) {
            godot::UtilityFunctions::push_error(godot::String(message.message().utf8().data()));
        } else if (level == ultralight::MessageLevel::kMessageLevel_Warning && logLevel <= LogLevel::WARNING) {
            godot::UtilityFunctions::push_warning(godot::String(message.message().utf8().data()));
        } else if (level == ultralight::MessageLevel::kMessageLevel_Debug && logLevel <= LogLevel::DEBUG) {
            godot::UtilityFunctions::print_rich(godot::String(message.message().utf8().data()));
        } else if (level == ultralight::MessageLevel::kMessageLevel_Info && logLevel <= LogLevel::INFO) {
            godot::UtilityFunctions::print(godot::String(message.message().utf8().data()));
        }
    }
};
} // namespace gdbind
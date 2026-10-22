#pragma once
#include <Ultralight/Ultralight.h>
#include <functional>
#include <godot_cpp/variant/utility_functions.hpp>
namespace gdbind {
struct ViewListener : public ultralight::ViewListener {
    std::function<void(ultralight::Cursor)> onCursor;
    std::function<void(const ultralight::ConsoleMessage &)> onConsole;
    void OnChangeCursor(ultralight::View *, ultralight::Cursor cursor) override {
        auto callback = onCursor;
        if (callback)
            callback(cursor);
    }
    std::function<ultralight::RefPtr<ultralight::View>()> onInspector;
    ultralight::RefPtr<ultralight::View> OnCreateInspectorView(ultralight::View *, bool,
                                                               const ultralight::String &) override {
        auto callback = onInspector;
        return callback ? callback() : nullptr;
    }
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
    virtual void OnAddConsoleMessage(ultralight::View *caller, const ultralight::ConsoleMessage &message) override {
        auto callback = onConsole;
        auto log_level = logLevel;
        if (callback)
            callback(message);
        auto level = message.level();
        if (level == ultralight::MessageLevel::kMessageLevel_Error) {
            godot::UtilityFunctions::push_error(godot::String(message.message().utf8().data()));
        } else if (level == ultralight::MessageLevel::kMessageLevel_Warning && log_level <= LogLevel::WARNING) {
            godot::UtilityFunctions::push_warning(godot::String(message.message().utf8().data()));
        } else if (level == ultralight::MessageLevel::kMessageLevel_Debug && log_level <= LogLevel::DEBUG) {
            godot::UtilityFunctions::print_rich(godot::String(message.message().utf8().data()));
        } else if (level == ultralight::MessageLevel::kMessageLevel_Info && log_level <= LogLevel::INFO) {
            godot::UtilityFunctions::print(godot::String(message.message().utf8().data()));
        } else if (level == ultralight::MessageLevel::kMessageLevel_Log && log_level <= LogLevel::INFO) {
            godot::UtilityFunctions::print(godot::String(message.message().utf8().data()));
        }
    }
};
} // namespace gdbind

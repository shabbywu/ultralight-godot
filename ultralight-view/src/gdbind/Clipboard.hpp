#pragma once
#include <Ultralight/platform/Clipboard.h>
#include <godot_cpp/classes/display_server.hpp>
namespace gdbind {
class GodotClipboard final : public ultralight::Clipboard {
  public:
    void Clear() override {
        godot::DisplayServer::get_singleton()->clipboard_set("");
    }
    ultralight::RefPtr<ultralight::ClipboardData> Read() override {
        auto text = godot::DisplayServer::get_singleton()->clipboard_get().utf8();
        return ultralight::ClipboardData::Create(ultralight::String(text.get_data(), text.length()));
    }
    void Write(ultralight::RefPtr<ultralight::ClipboardData> data) override {
        if (!data.get())
            return;
        auto utf8 = data->Get("text/plain").text().utf8();
        godot::DisplayServer::get_singleton()->clipboard_set(godot::String::utf8(utf8.data(), utf8.length()));
    }
};
} // namespace gdbind

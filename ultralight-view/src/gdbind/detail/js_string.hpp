#pragma once
#include <ulbind17/jsc/Bridge.hpp>
#include <godot_cpp/variant/string.hpp>

namespace gdbind {
class JSString : public ulbind17::jsc::String {
    explicit JSString(godot::Char16String text)
        : ulbind17::jsc::String(std::span<const char16_t>(
              reinterpret_cast<const char16_t *>(text.get_data()), text.length())) {}
  public:
    explicit JSString(const godot::String &text) : JSString(text.utf16()) {}
};
inline godot::String godot_string(JSStringRef text) {
    return godot::String::utf16(reinterpret_cast<const char16_t *>(JSStringGetCharactersPtr(text)),
                              JSStringGetLength(text));
}
} // namespace gdbind

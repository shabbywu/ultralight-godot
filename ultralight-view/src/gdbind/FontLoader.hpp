#pragma once
#include <Ultralight/platform/FontLoader.h>
#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <map>
#include <ulbind17/fonts/FreeUniversal-Regular.h>

namespace gdbind {
class GodotFontLoader : public ultralight::FontLoader {
  protected:
    std::map<godot::String, ultralight::RefPtr<ultralight::FontFile>> _fonts;

    static bool validate_ttf(const uint8_t *ptr, int64_t size) {
        if (size < 16) {
            return false;
        }

        // TTF flag: b'\x00\x01\x00\x00\x00'
        return ptr[0] == 0 && ptr[1] == 1 && ptr[2] == 0 && ptr[3] == 0 && ptr[4] == 0;
    }

  public:
    virtual ultralight::String fallback_font() const override {
        auto font = godot::ThemeDB::get_singleton()->get_fallback_font();
        return ultralight::String(font->get_font_name().utf8().ptr());
    }
    virtual ultralight::String fallback_font_for_characters(const ultralight::String &characters, int weight,
                                                            bool italic) const override {
        return fallback_font();
    }

    virtual ultralight::RefPtr<ultralight::FontFile> Load(const ultralight::String &family, int weight,
                                                          bool italic) override {
        auto theme = godot::ThemeDB::get_singleton()->get_default_theme();
        auto font = godot::ThemeDB::get_singleton()->get_default_theme()->get_font(family.utf8().data(), "");

        if (auto font_file = dynamic_cast<godot::FontFile *>(font.ptr()); font_file != nullptr) {
            if (auto it = _fonts.find(font->get_font_name()); it != _fonts.end()) {
                return it->second;
            }

            auto data = font_file->get_data();
            if (validate_ttf(data.ptr(), data.size())) {
                auto loaded = _fonts[font->get_font_name()] =
                    ultralight::FontFile::Create(ultralight::Buffer::CreateFromCopy(data.ptr(), data.size()));
                return loaded;
            }
        }

        // fallback
        auto &embeded = bin2cpp::getFreeUniversalRegularTtfFile();
        godot::String fallback(embeded.getFilename());
        if (auto it = _fonts.find(fallback); it != _fonts.end()) {
            return it->second;
        }
        auto loaded = _fonts[fallback] = ultralight::FontFile::Create(
            ultralight::Buffer::Create((void *)embeded.getBuffer(), embeded.getSize(), nullptr, nullptr));
        return loaded;
    }
};
} // namespace gdbind

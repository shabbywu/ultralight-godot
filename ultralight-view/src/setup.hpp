#pragma once
#include "gdbind/Filesystem.hpp"
#include "gdbind/Logger.hpp"
#include "gdbind/PackedByteArraySurface.hpp"
#include <Ultralight/Ultralight.h>

#include <ulbind17/fonts/FreeUniversal-Regular.h>
#include <ulbind17/platform/FontLoader.hpp>
#include <ulbind17/resources/cacert.h>
#include <ulbind17/resources/icudt67l.h>

namespace gdbind {
namespace setup {

struct EmbeddedResourceFileSystem : public GodotFileSystem {
  public:
    virtual bool FileExists(const ultralight::String &file_path) override {
        std::string p = file_path.utf8().data();
        {
            auto &icudt67l = bin2cpp::getIcudt67lDatFile();
            if (p == icudt67l.getFileName()) {
                return true;
            }
        }
        {
            auto &cacert = bin2cpp::getCacertPemFile();
            if (p == cacert.getFileName()) {
                return true;
            }
        }
        return GodotFileSystem::FileExists(file_path);
    }

    virtual ultralight::RefPtr<ultralight::Buffer> OpenFile(const ultralight::String &file_path) override {
        std::string p = file_path.utf8().data();
        {
            auto &icudt67l = bin2cpp::getIcudt67lDatFile();
            if (p == icudt67l.getFileName()) {
                return ultralight::Buffer::Create((void *)icudt67l.getBuffer(), icudt67l.getSize(), nullptr, nullptr);
            }
        }

        {
            auto &cacert = bin2cpp::getCacertPemFile();
            if (p == cacert.getFileName()) {
                return ultralight::Buffer::Create((void *)cacert.getBuffer(), cacert.getSize(), nullptr, nullptr);
            }
        }
        return GodotFileSystem::OpenFile(file_path);
    }
};

static void setup_ultralight_platform() {
    // Get the Platform singleton (maintains global library state)
    auto &platform = ultralight::Platform::instance();

    // Setup platform
    ultralight::Config my_config;
    platform.set_config(my_config);
    auto &font = bin2cpp::getFreeUniversalRegularTtfFile();
    platform.set_font_loader(new ulbind17::platform::MemoryFontLoader(
        {{font.getFileName(), ultralight::FontFile::Create(ultralight::Buffer::Create(
                                  (void *)font.getBuffer(), font.getSize(), nullptr, nullptr))}}));
    platform.set_file_system(new gdbind::setup::EmbeddedResourceFileSystem());

    platform.set_logger(GodotLogger::instance());
    platform.set_surface_factory(new PackedByteArraySurfaceFactory());
}
} // namespace setup
} // namespace gdbind

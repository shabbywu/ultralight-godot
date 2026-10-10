#pragma once
#include "gdbind/Filesystem.hpp"
#include "gdbind/FontLoader.hpp"
#include "gdbind/Logger.hpp"
#include "gdbind/gpu/GodotGPUDriver.hpp"
#include "gdbind/PackedByteArraySurface.hpp"
#include <Ultralight/Ultralight.h>

#include <ulbind17/resources/embedded/SDKResources.hpp>

namespace gdbind {
namespace setup {

struct EmbeddedResourceFileSystem : public GodotFileSystem {
    ulbind17::resources::embedded::SDKResources resources;
    explicit EmbeddedResourceFileSystem(const ultralight::String &prefix) : resources(prefix.utf8().data()) {}
    bool FileExists(const ultralight::String &path) override {
        return resources.FileExists(path) || GodotFileSystem::FileExists(path);
    }
    ultralight::RefPtr<ultralight::Buffer> OpenFile(const ultralight::String &path) override {
        auto resource = resources.OpenFile(path);
        return resource.get() ? resource : GodotFileSystem::OpenFile(path);
    }
};

static void setup_ultralight_platform() {
    // Get the Platform singleton (maintains global library state)
    auto &platform = ultralight::Platform::instance();

    // Setup platform
    ultralight::Config my_config;
    platform.set_config(my_config);
    platform.set_font_loader(new gdbind::GodotFontLoader());
    platform.set_file_system(new gdbind::setup::EmbeddedResourceFileSystem(my_config.resource_path_prefix));

    platform.set_logger(GodotLogger::instance());
    platform.set_gpu_driver(&GodotGPUDriver::instance());
    platform.set_surface_factory(new PackedByteArraySurfaceFactory());
}
} // namespace setup
} // namespace gdbind

#pragma once
#include "gdbind/Clipboard.hpp"
#include "gdbind/Filesystem.hpp"
#include "gdbind/FontLoader.hpp"
#include "gdbind/Logger.hpp"
#include "gdbind/PackedByteArraySurface.hpp"
#include "gdbind/gpu/GodotGPUDriver.hpp"
#include <Ultralight/Ultralight.h>

#include <memory>
#include <ulbind17/resources/FileSystemOverlay.hpp>
#include <ulbind17/resources/embedded/SDKResources.hpp>

namespace gdbind {
namespace setup {

using EmbeddedResourceFileSystem = ulbind17::resources::FileSystemOverlay<ulbind17::resources::embedded::SDKResources>;

static void setup_ultralight_platform() {
    // Get the Platform singleton (maintains global library state)
    auto &platform = ultralight::Platform::instance();

    // Setup platform
    ultralight::Config my_config;
    platform.set_config(my_config);
    static GodotClipboard clipboard;
    platform.set_clipboard(&clipboard);
    platform.set_font_loader(new gdbind::GodotFontLoader());
    // Created before the Renderer singleton and kept until after its shutdown.
    auto prefix = my_config.resource_path_prefix.utf8();
    static auto file_system = std::make_unique<EmbeddedResourceFileSystem>(
        ulbind17::resources::embedded::SDKResources(std::string(prefix.data(), prefix.length())),
        std::make_shared<GodotFileSystem>());
    platform.set_file_system(file_system.get());

    platform.set_logger(GodotLogger::instance());
    platform.set_gpu_driver(&GodotGPUDriver::instance());
    platform.set_surface_factory(new PackedByteArraySurfaceFactory());
}
} // namespace setup
} // namespace gdbind

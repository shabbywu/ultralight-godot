#pragma once
#include <Ultralight/Ultralight.h>

#include "Filesystem.hpp"
#include <AppCore/AppCore.h>
#include <ulbind17/fonts/FreeUniversal-Regular.h>
#include <ulbind17/platform/FontLoader.hpp>
#include <ulbind17/resources/cacert.h>
#include <ulbind17/resources/icudt67l.h>

#include "gen/inspector.h"
#include "gen/ui.h"

namespace mini {
namespace setup {

static void setup_ultralight_platform() {
    // Get the Platform singleton (maintains global library state)
    auto &platform = ultralight::Platform::instance();

    // Setup platform
    ultralight::Config my_config;
    platform.set_config(my_config);
    auto &font = bin2cpp::getFreeUniversalRegularTtfFile();
    platform.set_font_loader(ultralight::GetPlatformFontLoader());
    platform.set_file_system(new mini::PhysfsFileSystem("."));
}

static int setup_embeded_filesystem() {
    for (auto file : {&bin2cpp::getUiZipFile(), &bin2cpp::getInspectorZipFile()}) {
        if (!PHYSFS_mountMemory(file->getBuffer(), file->getSize(), NULL, file->getFileName(), "/", 0)) {
            std::cerr << std::format("failed to mount {}, Reason: [{}]", file->getFileName(), PHYSFS_getLastError())
                      << std::endl;
            return -1;
        }
    }
    return 0;
}

} // namespace setup
} // namespace mini

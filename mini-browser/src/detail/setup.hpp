#pragma once
#include <Ultralight/Ultralight.h>

#include "Filesystem.hpp"
#include <AppCore/AppCore.h>


#include "generated_assets/inspector.h"
#include "generated_assets/ui.h"

namespace mini {
namespace setup {

static void setup_ultralight_platform() {
    // Get the Platform singleton (maintains global library state)
    auto &platform = ultralight::Platform::instance();

    // Setup platform
    ultralight::Config my_config;
    platform.set_config(my_config);
    // Install the filesystem before AppCore initializes its native font loader/ICU.
    // App::Create preserves this handler and supplies the platform font loader.
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

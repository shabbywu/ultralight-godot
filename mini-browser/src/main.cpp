#include "Browser.h"
#include "detail/physfs.hpp"
#include "detail/setup.hpp"
#include <AppCore/Dialogs.h>
#include <string_view>

int main(int argc, char **argv) {
    bool gpu = false;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg == "--gpu") gpu = true;
        else return 2;
    }
    mini::setup::setup_ultralight_platform();
    if (auto errorcode = mini::setup::setup_embeded_filesystem(); errorcode != 0) {
        return errorcode;
    }

    Browser browser(gpu, false, true);
    browser.Run();

    return 0;
}

#include "Browser.h"
#include "detail/physfs.hpp"
#include "detail/setup.hpp"
#include <AppCore/Dialogs.h>

int main() {
    mini::setup::setup_ultralight_platform();
    if (auto errorcode = mini::setup::setup_embeded_filesystem(); errorcode != 0) {
        return errorcode;
    }

    Browser browser(false, false, true);
    browser.Run();

    return 0;
}

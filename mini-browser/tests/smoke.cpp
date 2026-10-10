#include "src/Browser.h"
#include "src/detail/physfs.hpp"
#include "src/detail/setup.hpp"
#include <AppCore/Layout.h>
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void CollectPanels(RefPtr<Container> container, std::vector<RefPtr<Panel>> &panels) {
    for (int i = 0; i < container->child_count(); ++i) {
        auto child = container->child_at(i);
        if (auto panel = child->AsPanel()) panels.push_back(panel);
        else if (auto nested = child->AsContainer()) CollectPanels(nested, panels);
    }
}

class SmokeTest : public AppListener {
  public:
    explicit SmokeTest(Browser &browser) : browser_(browser) {
        browser_.app()->set_listener(this);
    }

    ~SmokeTest() {
        browser_.app()->set_listener(nullptr);
    }

    int Run() {
        started_ = changed_ = std::chrono::steady_clock::now();
        browser_.Run();
        return result_;
    }

    void OnUpdate() override {
        auto now = std::chrono::steady_clock::now();
        if (now - started_ > std::chrono::seconds(45)) {
            std::cerr << "MINI_BROWSER FAIL: timed out at step " << step_ << '\n';
            browser_.app()->Quit();
            return;
        }
        if (now - changed_ < std::chrono::milliseconds(500)) return;

        // The layout exposes the toolbar, content and optional inspector in this order.
        std::vector<RefPtr<Panel>> panels;
        CollectPanels(browser_.window()->layout(), panels);
        if (panels.size() < 2) return;
        auto toolbar = panels[0]->view();
        auto page = panels[1]->view();
        RefPtr<Panel> inspector = panels.size() > 2 ? panels[2] : nullptr;
        if (page->is_loading()) return;

        switch (step_) {
        case 0:
            if (Evaluate(page, "!!document.body") != "true" ||
                Evaluate(toolbar, "typeof OnRequestChangeURL === 'function'") != "true") return;
            page_height_ = page->height();
            Evaluate(toolbar, "OnRequestChangeURL('file:///ui/welcome.html?smoke=1')");
            break;
        case 1:
            if (!page->CanGoBack()) return;
            Evaluate(toolbar, "OnBack()");
            break;
        case 2:
            if (!page->CanGoForward()) return;
            Evaluate(toolbar, "OnForward()");
            break;
        case 3:
            if (Evaluate(page, "location.search") != "?smoke=1") return;
            Evaluate(toolbar, "OnRefresh()");
            break;
        case 4:
            Evaluate(toolbar, "OnToggleTools()");
            break;
        case 5:
            if (!inspector || inspector->is_hidden() || inspector->view()->is_loading() ||
                Evaluate(inspector->view(), "!!document.body && document.body.children.length > 0") != "true") return;
            if (!page->width() || !page->height() ||
                !inspector->view()->width() || !inspector->view()->height()) return;
            if (page->height() >= page_height_) return;
            Evaluate(toolbar, "OnToggleTools()");
            break;
        case 6:
            if (!inspector || !inspector->is_hidden() || page->height() != page_height_) return;
            Evaluate(toolbar, "OnToggleTools()");
            break;
        case 7:
            if (!inspector || inspector->is_hidden() || page->height() >= page_height_) return;
            result_ = 0;
            std::cout << "MINI_BROWSER PASS: toolbar, navigation, reload, inspector panels\n";
            browser_.app()->Quit();
            return;
        }
        ++step_;
        changed_ = now;
    }

  private:
    static std::string Evaluate(RefPtr<View> view, const char *code) {
        String error;
        auto result = view->EvaluateScript(code, &error);
        if (!error.empty()) {
            std::cerr << "MINI_BROWSER script error: " << error.utf8().data() << '\n';
            return {};
        }
        return result.utf8().data();
    }

    Browser &browser_;
    int step_ = 0, result_ = 1;
    uint32_t page_height_ = 0;
    std::chrono::steady_clock::time_point started_, changed_;
};

} // namespace

int main(int argc, char **argv) {
    bool gpu = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--gpu") gpu = true;
        else return 2;
    }
    mini::setup::setup_ultralight_platform();
    if (auto errorcode = mini::setup::setup_embeded_filesystem(); errorcode != 0) {
        return errorcode;
    }
    Browser browser(gpu, false, false);
    SmokeTest test(browser);
    return test.Run();
}

#pragma once
#include <AppCore/AppCore.h>
#include <memory>

using namespace ultralight;

class UI;

class Browser {
  public:
    Browser(bool use_gpu, bool force_repaint, bool show_stats);
    ~Browser();

    void Run();

    void SetTitle(const String &title);

    RefPtr<App> app() {
        return app_;
    }
    RefPtr<Window> window() {
        return window_;
    }

  protected:
    RefPtr<App> app_;
    RefPtr<Window> window_;
    std::unique_ptr<UI> ui_;
};

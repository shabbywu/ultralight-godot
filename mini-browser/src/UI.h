#pragma once
#include "Browser.h"
#include "Page.h"
#include <AppCore/AppCore.h>
#include <ulbind17/ulbind17.hpp>
#include <memory>

using namespace ultralight;

class Console;

/**
 * Browser UI implementation. Renders the toolbar/addressbar/tabs in top pane.
 */
class UI : public WindowListener, public LoadListener, public ViewListener {
  public:
    UI(Browser *browser);
    ~UI();

    // Inherited from WindowListener
    virtual bool OnKeyEvent(ultralight::Window *window, const ultralight::KeyEvent &evt) override;
    virtual bool OnMouseEvent(ultralight::Window *window, const ultralight::MouseEvent &evt) override;
    virtual void OnClose(ultralight::Window *window) override;
    virtual void OnResize(ultralight::Window *window, double width, double height) override;

    // Inherited from LoadListener
    void OnBeginLoading(View *, uint64_t, bool is_main_frame, const String &) override {
        if (is_main_frame) context_ = {};
    }
    virtual void OnDOMReady(View *caller, uint64_t frame_id, bool is_main_frame, const String &url) override;

    // Inherited from ViewListener
    virtual void OnChangeCursor(ultralight::View *caller, Cursor cursor) override {
        SetCursor(cursor);
    }

    // Called by UI JavaScript
    void OnBack();
    void OnForward();
    void OnRefresh();
    void OnStop();
    void OnToggleTools();
    void OnRequestChangeURL(const ultralight::String &url);

    RefPtr<Window> window() {
        return browser_->window();
    }

  protected:
    void CreatePage();
    void UpdatePageNavigation(bool is_loading, bool can_go_back, bool can_go_forward);

    void SetLoading(bool is_loading);
    void SetCanGoBack(bool can_go_back);
    void SetCanGoForward(bool can_go_forward);
    void SetTitle(const String &title);
    void SetURL(const String &url);
    void SetCursor(Cursor cursor);
    void UpdateToolbar(const char *name, const ulbind17::js::Value &value);

    Page *page() {
        return page_.get();
    }

    RefPtr<View> view() {
        return panel_->view();
    }

    Browser *browser_;
    RefPtr<Panel> panel_;
    RefPtr<Container> body_;
    std::unique_ptr<Page> page_;
    std::unique_ptr<ulbind17::Bindings> bindings_;
    ulbind17::js::Context context_;
    Cursor cur_cursor_;

    friend class Page;
};

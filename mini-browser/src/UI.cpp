#include "UI.h"
#include <iostream>
#include <stdexcept>

UI::UI(Browser *browser) : browser_(browser), cur_cursor_(Cursor::kCursor_Pointer) {
    panel_ = window()->AddPanel({ .size = "41px", .fixed = true });
    body_ = window()->layout()->AddColumn({ .resizable = true });
    body_->SetDividerStyle({ .hit_width = 10 });
    bindings_ = std::make_unique<ulbind17::Bindings>("browser");
    bindings_->bindFunc("OnBack", ulbind17::js::Bind(this, &UI::OnBack));
    bindings_->bindFunc("OnForward", ulbind17::js::Bind(this, &UI::OnForward));
    bindings_->bindFunc("OnRefresh", ulbind17::js::Bind(this, &UI::OnRefresh));
    bindings_->bindFunc("OnStop", ulbind17::js::Bind(this, &UI::OnStop));
    bindings_->bindFunc("OnToggleTools", ulbind17::js::Bind(this, &UI::OnToggleTools));
    bindings_->bindFunc("OnRequestChangeURL", [this](std::string url) { OnRequestChangeURL(url.c_str()); });
    if (!bindings_->AttachTo(view().get())) throw std::runtime_error("Unable to bind browser API");
    view()->set_load_listener(this);
    view()->set_view_listener(this);
    view()->LoadURL("file:///ui/ui.html");
}
UI::~UI() {
    bindings_.reset(); context_ = {}; page_.reset();
    view()->set_load_listener(nullptr); view()->set_view_listener(nullptr);
    window()->layout()->Remove(body_); window()->layout()->Remove(panel_);
}
bool UI::OnKeyEvent(ultralight::Window *, const ultralight::KeyEvent &evt) {
    if (evt.virtual_key_code == KeyCodes::GK_F2) {
        if (evt.type == KeyEvent::kType_RawKeyDown)
            App::instance()->renderer()->StartRemoteInspectorServer("0.0.0.0", 7676);
        return false;
    }
    return true;
}
bool UI::OnMouseEvent(ultralight::Window *, const ultralight::MouseEvent &) { return true; }

void UI::OnClose(ultralight::Window *window) {
    App::instance()->Quit();
}

void UI::OnResize(ultralight::Window *, double, double) {
    // Panel layout owns logical sizing, DPI conversion and divider hit testing.
}

void UI::OnDOMReady(View *caller, uint64_t frame_id, bool is_main_frame, const String &url) {
    if (!is_main_frame) return;
    context_ = ulbind17::js::Context(caller);
    auto global = ulbind17::Object::GetGlobalObject(context_);
    auto api = global.value().GetProperty("browser");
    if (!api) { std::cerr << "Browser JS: " << api.error().message() << '\n'; return; }
    // Retain the toolbar's existing global callback names while the SDK owns
    // native binding lifetime and navigation reinjection under browser.*.
    for (const auto *name : {"OnBack", "OnForward", "OnRefresh", "OnStop", "OnToggleTools", "OnRequestChangeURL"}) {
        auto function = api.value().GetProperty(name);
        if (!function) { std::cerr << "Browser JS: " << function.error().message() << '\n'; return; }
        auto assigned = global.set(name, function.value());
        if (!assigned) { std::cerr << "Browser JS: " << assigned.error().message() << '\n'; return; }
    }
    if (!page_) CreatePage();
}

void UI::OnBack() {
    if (page())
        page()->view()->GoBack();
}

void UI::OnForward() {
    if (page())
        page()->view()->GoForward();
}

void UI::OnRefresh() {
    if (page())
        page()->view()->Reload();
}

void UI::OnStop() {
    if (page())
        page()->view()->Stop();
}

void UI::OnToggleTools() {
    if (page())
        page()->ToggleInspector();
}

void UI::OnRequestChangeURL(const ultralight::String &url) {
    if (page()) page()->view()->LoadURL(url);
}
void UI::CreatePage() {
    page_ = std::make_unique<Page>(this);
    page_->view()->LoadURL("file:///ui/welcome.html");
}

void UI::UpdatePageNavigation(bool is_loading, bool can_go_back, bool can_go_forward) {
    SetLoading(is_loading);
    SetCanGoBack(can_go_back);
    SetCanGoForward(can_go_forward);
}

void UI::UpdateToolbar(const char *name, const ulbind17::js::Value &value) {
    if (!context_) return;
    auto result = ulbind17::Object::GetGlobalObject(context_).call<void>(name, value);
    if (!result) std::cerr << "Browser JS: " << result.error().message() << '\n';
}

void UI::SetLoading(bool is_loading) {
    if (context_) UpdateToolbar("updateLoading", context_.Make(is_loading));
}

void UI::SetCanGoBack(bool can_go_back) {
    if (context_) UpdateToolbar("updateBack", context_.Make(can_go_back));
}

void UI::SetCanGoForward(bool can_go_forward) {
    if (context_) UpdateToolbar("updateForward", context_.Make(can_go_forward));
}

void UI::SetTitle(const String &title) {
    browser_->SetTitle(title);
}

void UI::SetURL(const ultralight::String &url) {
    if (context_) UpdateToolbar("updateURL", context_.Make(std::string(url.utf8().data(), url.utf8().length())));
}

void UI::SetCursor(ultralight::Cursor cursor) {
    cur_cursor_ = cursor;

    if (App::instance())
        window()->SetCursor(cursor);
}

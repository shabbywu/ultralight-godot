#include "Page.h"
#include "UI.h"
#include <iostream>
#include <string>
#include <ulbind17/jsc/Bridge.hpp>

Page::Page(UI *ui) : ui_(ui) {
    panel_ = ui_->body_->AddPanel();
    view()->set_view_listener(this);
    view()->set_load_listener(this);
    view()->set_download_listener(this);
    view()->set_network_listener(this);
}

Page::~Page() {
    view()->set_network_listener(nullptr);
    view()->set_download_listener(nullptr);
    view()->set_view_listener(nullptr);
    view()->set_load_listener(nullptr);
    if (inspector_panel_) ui_->body_->Remove(inspector_panel_);
    ui_->body_->Remove(panel_);
}

void Page::Show() {
    panel_->Show();
    panel_->Focus();

    if (inspector_panel_)
        inspector_panel_->Show();
}

void Page::Hide() {
    panel_->Hide();
    if (ui_->window()->focused_panel() == panel_) ui_->window()->ClearFocus();

    if (inspector_panel_)
        inspector_panel_->Hide();
}

void Page::ToggleInspector() {
    if (!inspector_panel_) {
        view()->CreateLocalInspectorView();
    } else {
        if (inspector_panel_->is_hidden()) {
            inspector_panel_->Show();
        } else {
            inspector_panel_->Hide();
        }
    }

}

bool Page::IsInspectorShowing() const {
    if (!inspector_panel_)
        return false;

    return !inspector_panel_->is_hidden();
}

void Page::OnChangeTitle(View *caller, const String &title) {
    ui_->SetTitle(title);
}

void Page::OnChangeURL(View *caller, const String &url) {
    ui_->SetURL(url);
}

void Page::OnChangeTooltip(View *caller, const String &tooltip) {
}

void Page::OnChangeCursor(View *caller, Cursor cursor) {
    ui_->SetCursor(cursor);
}

void Page::OnAddConsoleMessage(View *caller, const ConsoleMessage &msg) {
    std::cout << "[OnAddConsoleMessage]\n\t" << "\n\tsource:\t" << (uint32_t)msg.source() << "\n\ttype:\t"
              << (uint32_t)msg.type() << "\n\tlevel:\t" << (uint32_t)msg.level() << "\n\tmessage:\t"
              << msg.message().utf8().data() << "\n\tline_number:\t" << msg.line_number() << "\n\tcolumn_number:\t"
              << msg.column_number() << "\n\tsource_id:\t" << msg.source_id().utf8().data() << "\n\tnum_arguments:\t"
              << msg.num_arguments() << std::endl;

    for (uint32_t i = 0; i < msg.num_arguments(); ++i) {
        std::cout << "\n\t[" << i << "]:\t";
        try { std::cout << ulbind17::jsc::text(msg.argument_context(), msg.argument_at(i)); }
        catch (const std::exception &error) { std::cout << "[conversion failed: " << error.what() << ']'; }
    }
    std::cout << std::endl;
}

RefPtr<View> Page::OnCreateChildView(ultralight::View *caller, const String &opener_url, const String &target_url,
                                     bool is_popup, const IntRect &popup_rect) {

    // TODO: handle child view in new window
    return nullptr;
}

RefPtr<View> Page::OnCreateInspectorView(ultralight::View *caller, bool is_local, const String &inspected_url) {
    if (inspector_panel_)
        return nullptr;

    inspector_panel_ = ui_->body_->AddPanel({ .size = "50%", .min_size = "80px" });

    inspector_panel_->Show();

    return inspector_panel_->view();
}

void Page::OnBeginLoading(View *caller, uint64_t frame_id, bool is_main_frame, const String &url) {
    ui_->UpdatePageNavigation(caller->is_loading(), caller->CanGoBack(), caller->CanGoForward());

    std::cout << "[OnBeginLoading]\n\t" << "\n\tframe_id:\t" << frame_id << "\n\tis_main_frame:\t" << is_main_frame
              << "\n\turl:\t" << url.utf8().data() << std::endl;
}

void Page::OnFinishLoading(View *caller, uint64_t frame_id, bool is_main_frame, const String &url) {
    ui_->UpdatePageNavigation(caller->is_loading(), caller->CanGoBack(), caller->CanGoForward());

    std::cout << "[OnFinishLoading]\n\t" << "\n\tframe_id:\t" << frame_id << "\n\tis_main_frame:\t" << is_main_frame
              << "\n\turl:\t" << url.utf8().data() << std::endl;
}

void Page::OnFailLoading(View *caller, uint64_t frame_id, bool is_main_frame, const String &url,
                         const String &description, const String &error_domain, int error_code) {
    if (is_main_frame) {
        char error_code_str[16];
        sprintf(error_code_str, "%d", error_code);

        String html_string = "<html><head><style>";
        html_string += "* { font-family: sans-serif; }";
        html_string += "body { background-color: #CCC; color: #555; padding: 4em; }";
        html_string += "dt { font-weight: bold; padding: 1em; }";
        html_string += "</style></head><body>";
        html_string += "<h2>A Network Error was Encountered</h2>";
        html_string += "<dl>";
        html_string += "<dt>URL</dt><dd>" + url + "</dd>";
        html_string += "<dt>Description</dt><dd>" + description + "</dd>";
        html_string += "<dt>Error Domain</dt><dd>" + error_domain + "</dd>";
        html_string += "<dt>Error Code</dt><dd>" + String(error_code_str) + "</dd>";
        html_string += "</dl></body></html>";

        view()->LoadHTML(html_string);
    }

    std::cout << "[OnFailLoading]\n\t" << "\n\tframe_id:\t" << frame_id << "\n\tis_main_frame:\t" << is_main_frame
              << "\n\turl:\t" << url.utf8().data() << "\n\tdescription:\t" << description.utf8().data()
              << "\n\terror_domain:\t" << error_domain.utf8().data() << "\n\terror_code:\t" << error_code << std::endl;
}

void Page::OnWindowObjectReady(ultralight::View *caller, uint64_t frame_id, bool is_main_frame, const String &url) {
    std::cout << "[OnWindowObjectReady]\n\t" << "\n\tframe_id:\t" << frame_id << "\n\tis_main_frame:\t" << is_main_frame
              << "\n\turl:\t" << url.utf8().data() << std::endl;
}

void Page::OnDOMReady(ultralight::View *caller, uint64_t frame_id, bool is_main_frame, const String &url) {
    std::cout << "[OnDOMReady]\n\t" << "\n\tframe_id:\t" << frame_id << "\n\tis_main_frame:\t" << is_main_frame
              << "\n\turl:\t" << url.utf8().data() << std::endl;
}

void Page::OnUpdateHistory(View *caller) {
    ui_->UpdatePageNavigation(caller->is_loading(), caller->CanGoBack(), caller->CanGoForward());
}

bool Page::OnRequestDownload(View *caller, DownloadId id, const String &url) {
    printf("Page::OnRequestDownload [%d] %s\n", id, url.utf8().data());
    return true;
}

void Page::OnBeginDownload(View *caller, DownloadId id, const String &url, const String &filename,
                           int64_t expected_content_length) {
    printf("Page::OnBeginDownload [%d] %s\n", id, url.utf8().data());
}

void Page::OnReceiveDataForDownload(View *caller, DownloadId id, RefPtr<Buffer> data) {
    printf("Page::OnReceiveDataForDownload [%d]\n", id);
}

void Page::OnFinishDownload(View *caller, DownloadId id) {
    printf("Page::OnFinishDownload [%d]\n", id);
}

void Page::OnFailDownload(View *caller, DownloadId id) {
    printf("Page::OnFailDownload [%d]\n", id);
}

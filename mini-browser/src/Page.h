#pragma once
#include <AppCore/AppCore.h>
#include <Ultralight/Listener.h>

class UI;
using namespace ultralight;

/**
 * Browser Page implementation. Renders the actual page content in bottom pane.
 */
class Page : public ViewListener, public LoadListener, public DownloadListener, public NetworkListener {
  public:
    Page(UI *ui, uint32_t width, uint32_t height, int x, int y);
    ~Page();

    void set_ready_to_close(bool ready) {
        ready_to_close_ = ready;
    }
    bool ready_to_close() {
        return ready_to_close_;
    }

    RefPtr<View> view() {
        return overlay_->view();
    }

    void Show();

    void Hide();

    void ToggleInspector();

    bool IsInspectorShowing() const;

    IntRect GetInspectorResizeDragHandle() const;

    int GetInspectorHeight() const;

    void SetInspectorHeight(int height);

    void Resize(uint32_t width, uint32_t height);

    // Inherited from ViewListener
    virtual void OnChangeTitle(View *caller, const String &title) override;
    virtual void OnChangeURL(View *caller, const String &url) override;
    virtual void OnChangeTooltip(View *caller, const String &tooltip) override;
    virtual void OnChangeCursor(View *caller, Cursor cursor) override;
    virtual void OnAddConsoleMessage(View *caller, const ConsoleMessage &msg) override;
    virtual RefPtr<View> OnCreateChildView(ultralight::View *caller, const String &opener_url, const String &target_url,
                                           bool is_popup, const IntRect &popup_rect) override;
    virtual RefPtr<View> OnCreateInspectorView(ultralight::View *caller, bool is_local,
                                               const String &inspected_url) override;

    // Inherited from LoadListener
    virtual void OnBeginLoading(View *caller, uint64_t frame_id, bool is_main_frame, const String &url) override;
    virtual void OnFinishLoading(View *caller, uint64_t frame_id, bool is_main_frame, const String &url) override;
    virtual void OnFailLoading(View *caller, uint64_t frame_id, bool is_main_frame, const String &url,
                               const String &description, const String &error_domain, int error_code) override;
    virtual void OnWindowObjectReady(View *caller, uint64_t frame_id, bool is_main_frame, const String &url) override;
    virtual void OnDOMReady(View *caller, uint64_t frame_id, bool is_main_frame, const String &url) override;
    virtual void OnUpdateHistory(View *caller) override;

    // Inherited from DownloadListener
    virtual DownloadId NextDownloadId(View *caller) override {
        return next_download_id_++;
    }
    virtual bool OnRequestDownload(View *caller, DownloadId id, const String &url) override;
    virtual void OnBeginDownload(View *caller, DownloadId id, const String &url, const String &filename,
                                 int64_t expected_content_length) override;

    virtual void OnReceiveDataForDownload(View *caller, DownloadId id, RefPtr<Buffer> data) override;
    virtual void OnFinishDownload(View *caller, DownloadId id) override;
    virtual void OnFailDownload(View *caller, DownloadId id) override;

    // Inherited from NetworkListener
    virtual bool OnNetworkRequest(View *caller, NetworkRequest &request) override {
        return true;
    }

  protected:
    UI *ui_;
    RefPtr<Overlay> overlay_;
    RefPtr<Overlay> inspector_overlay_;
    bool ready_to_close_ = false;
    uint32_t container_width_, container_height_;
    DownloadId next_download_id_ = 0;
};

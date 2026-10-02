#pragma once

#include "listener/DownloadListener.hpp"
#include "listener/LoadListener.hpp"
#include "listener/NetworkListener.hpp"
#include "listener/ViewListener.hpp"
#include <Ultralight/Ultralight.h>
#include <memory>
#include <chrono>
#include <ulbind17/ulbind17.hpp>

using namespace ultralight;
using namespace godot;

namespace gdbind {
class UltralightRenderer final {
  public:
    struct Listeners {
        ViewListener view;
        LoadListener load;
        NetworkListener network;
        DownloadListener download;
    };
    struct CreateViewResult {
        ultralight::RefPtr<ultralight::View> view;
        std::shared_ptr<Listeners> listeners;
    };

  private:
    ultralight::RefPtr<ultralight::Renderer> render;
    double render_us = 0;
    UltralightRenderer() = default;
    static UltralightRenderer &instance() { static UltralightRenderer value; return value; }
    UltralightRenderer(const UltralightRenderer &other) = delete;
    void operator=(const UltralightRenderer &other) = delete;

  public:
    static UltralightRenderer *get_singleton() {
        auto &value = instance();
        if (value.render.get() == nullptr) value.render = ultralight::Renderer::Create();
        return &value;
    }
    static void shutdown_existing() { instance().shutdown(); }
    static double render_time_us() { return instance().render_us; }

    void shutdown() {
        render = nullptr;
    }

  public:
    /// @brief Update timers and dispatch callbacks.
    /// You should call this as often as you can from your application's run loop.
    void updateLogic() {
        render->Update();
    }

    /// @brief Render all active views to their respective surfaces and render targets.
    /// You should call this once per frame (usually in synchrony with the monitor's refresh rate).
    void updateFrame() {
        auto start = std::chrono::steady_clock::now();
        render->RefreshDisplay(0);
        render->Render();
        render_us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now()-start).count();
    }

    ///
    /// Create a new View to load and display web pages in.
    ///
    /// Views are similar to a tab in a browser. They have certain dimensions but are rendered to an
    /// offscreen surface and must be forwarded all input events.
    ///
    /// @param  width    The initial width, in pixels.
    ///
    /// @param  height   The initial height, in pixels.
    ///
    /// @param  config   Configuration details for the View.
    ///
    /// @param  session  The session to store local data in. Pass a nullptr to use the default
    ///                  session.
    ///
    /// @return  Returns a ref-pointer to a new View instance.
    ///
    CreateViewResult createView(uint32_t width, uint32_t height, const ViewConfig &config, RefPtr<Session> session) {
        auto view = render->CreateView(width, height, config, session);
        CreateViewResult result{view, std::make_shared<Listeners>()};
        view->set_view_listener(&result.listeners->view);
        view->set_load_listener(&result.listeners->load);
        view->set_network_listener(&result.listeners->network);
        view->set_download_listener(&result.listeners->download);
        return result;
    }

    ///
    /// Start the remote inspector server.
    ///
    /// While the remote inspector is active, Views that are loaded into this renderer
    /// will be able to be remotely inspected from another Ultralight instance either locally
    /// (another app on same machine) or remotely (over the network) by navigating a View to:
    ///
    /// \code
    ///   inspector://<ADDRESS>:<PORT>
    /// \endcode
    ///
    /// @return  Returns whether the server started successfully or not.
    ///
    bool startRemoteInspectorServer(const char *address, uint16_t port) {
        auto result = render->StartRemoteInspectorServer(address, port);
        godot::UtilityFunctions::print("StartRemoteInspectorServer at ", address, ":", port, " with result ", result);
        return result;
    }
};
} // namespace gdbind

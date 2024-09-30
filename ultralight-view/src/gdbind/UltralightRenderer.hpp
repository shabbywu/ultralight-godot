#pragma once

#include "listener/DownloadListener.hpp"
#include "listener/LoadListener.hpp"
#include "listener/NetworkListener.hpp"
#include "listener/ViewListener.hpp"
#include <Ultralight/Ultralight.h>
#include <memory>
#include <ulbind17/ulbind17.hpp>

using namespace ultralight;
using namespace godot;

namespace gdbind {
class UltralightRenderer final {
    struct CreateViewResult {
        ultralight::RefPtr<ultralight::View> view;
        ViewListener *viewListener;
        LoadListener *loadListener;
    };

  private:
    ultralight::RefPtr<ultralight::Renderer> render;
    UltralightRenderer() {
        render = ultralight::Renderer::Create();
    }
    UltralightRenderer(const UltralightRenderer &other) = delete;
    void operator=(const UltralightRenderer &other) = delete;

  public:
    static UltralightRenderer *get_singleton() {
        static UltralightRenderer instance;
        // re-create render after shutdown()
        if (instance.render.get() == nullptr) {
            instance.render = ultralight::Renderer::Create();
        }
        return &instance;
    }

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
        render->RefreshDisplay(0);
        render->Render();
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
        CreateViewResult result = {
            .view = view,
            .viewListener = new ViewListener,
            .loadListener = new LoadListener,
        };
        view->set_view_listener(result.viewListener);
        view->set_load_listener(result.loadListener);
        view->set_network_listener(new NetworkListener);
        view->set_download_listener(new DownloadListener);
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

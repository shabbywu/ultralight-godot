#pragma once

#include "listener/LoadListener.hpp"
#include "listener/ViewListener.hpp"
#include <Ultralight/Ultralight.h>
#include <memory>
#include <ulbind17/ulbind17.hpp>

using namespace ultralight;
using namespace godot;

namespace gdbind {
class UltralightRenderer {
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
        return &instance;
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
        return result;
    }
};
} // namespace gdbind

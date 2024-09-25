#pragma once
#include "UltralightRenderer.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
using namespace ultralight;
using namespace godot;

namespace gdbind {
class UltralightSingleton : public Object {
    GDCLASS(UltralightSingleton, Object);

  public:
    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("update_logic"), &UltralightSingleton::updateLogic);
        ClassDB::bind_method(D_METHOD("update_frame"), &UltralightSingleton::updateFrame);
        ClassDB::bind_method(D_METHOD("set_process", "process"), &UltralightSingleton::setProcess);
        ClassDB::bind_method(D_METHOD("start_remove_inspector_server", "address", "port"),
                             &UltralightSingleton::startRemoteInspectorServer);
    }

    // get_singleton from c++ side
    static UltralightSingleton *get_singleton() {
        auto singleton = (UltralightSingleton *)godot::Engine::get_singleton()->get_singleton("UltralightSingleton");
        singleton->init();
        return singleton;
    }

  protected:
    bool inited = false;
    std::thread daemon;
    void init() {
        if (!inited) {
            RenderingServer::get_singleton()->connect("frame_pre_draw", Callable(this, "update_frame"));
            auto tree = dynamic_cast<godot::SceneTree *>(godot::Engine::get_singleton()->get_main_loop());
            tree->connect("process_frame", Callable(this, "update_logic"));
            inited = true;
        }
    }

    void deinit() {
        if (inited) {
            RenderingServer::get_singleton()->disconnect("frame_pre_draw", Callable(this, "update_frame"));
            auto tree = dynamic_cast<godot::SceneTree *>(godot::Engine::get_singleton()->get_main_loop());
            tree->disconnect("process_frame", Callable(this, "update_logic"));
        }
    }

    bool process = true;
    void setProcess(bool process) {
        if (this->process == process)
            return;

        auto tree = dynamic_cast<godot::SceneTree *>(godot::Engine::get_singleton()->get_main_loop());
        this->process = process;
        if (this->process) {
            tree->connect("process_frame", Callable(this, "update_logic"));
        } else {
            tree->disconnect("process_frame", Callable(this, "update_logic"));
        }
    }

#pragma region proxy to UltralightRenderer
  public:
    /// @brief Update timers and dispatch callbacks.
    /// You should call this as often as you can from your application's run loop.
    void updateLogic() {
        UltralightRenderer::get_singleton()->updateLogic();
    }

    /// @brief Render all active views to their respective surfaces and render targets.
    /// You should call this once per frame (usually in synchrony with the monitor's refresh rate).
    void updateFrame() {
        UltralightRenderer::get_singleton()->updateFrame();
    }

    auto createView(uint32_t width, uint32_t height, const ViewConfig &config, RefPtr<Session> session) {
        return UltralightRenderer::get_singleton()->createView(width, height, std::move(config), session);
    }

    bool startRemoteInspectorServer(godot::String address, unsigned int port) {
        return UltralightRenderer::get_singleton()->startRemoteInspectorServer(address.utf8().ptr(), port);
    }

    auto getRender() {
        return UltralightRenderer::get_singleton();
    }

#pragma endregion
};
} // namespace gdbind

#pragma once
#include "UltralightRenderer.hpp"
#include "gpu/GodotGPUDriver.hpp"

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
    virtual ~UltralightSingleton() {
    }

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
        if (singleton) singleton->init();
        return singleton;
    }

  protected:
    bool inited = false;
    bool stopped = false;
    bool process = true;
    void init() {
        if (inited || stopped) return;
        auto tree = dynamic_cast<godot::SceneTree *>(godot::Engine::get_singleton()->get_main_loop());
        if (!tree) return;
        RenderingServer::get_singleton()->connect("frame_pre_draw", Callable(this, "update_frame"));
        if (process) tree->connect("process_frame", Callable(this, "update_logic"));
        inited = true;
    }
    void deinit() {
        if (!inited) return;
        auto rs = RenderingServer::get_singleton();
        if (rs && rs->is_connected("frame_pre_draw", Callable(this, "update_frame")))
            rs->disconnect("frame_pre_draw", Callable(this, "update_frame"));
        auto tree = dynamic_cast<godot::SceneTree *>(godot::Engine::get_singleton()->get_main_loop());
        if (tree && tree->is_connected("process_frame", Callable(this, "update_logic")))
            tree->disconnect("process_frame", Callable(this, "update_logic"));
        inited = false;
    }
    void setProcess(bool enabled) {
        if (process == enabled) return;
        process = enabled;
        if (!inited || stopped) return;
        auto tree = dynamic_cast<godot::SceneTree *>(godot::Engine::get_singleton()->get_main_loop());
        if (!tree) return;
        if (process) tree->connect("process_frame", Callable(this, "update_logic"));
        else if (tree->is_connected("process_frame", Callable(this, "update_logic")))
            tree->disconnect("process_frame", Callable(this, "update_logic"));
    }

#pragma region proxy to UltralightRenderer
  public:
    /// @brief Update timers and dispatch callbacks.
    /// You should call this as often as you can from your application's run loop.
    void updateLogic() {
        if (!stopped) UltralightRenderer::get_singleton()->updateLogic();
    }

    /// @brief Render all active views to their respective surfaces and render targets.
    /// You should call this once per frame (usually in synchrony with the monitor's refresh rate).
    void updateFrame() {
        if (!stopped) UltralightRenderer::get_singleton()->updateFrame();
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

  public:
    void shutdown() {
        stopped = true;
        deinit();
        UltralightRenderer::shutdown_existing();
        GodotGPUDriver::instance().shutdown();
    }
};
} // namespace gdbind

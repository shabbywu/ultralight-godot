#pragma once
#include <memory>
#include <chrono>
#include <limits>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <Ultralight/Ultralight.h>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/texture_rect.hpp>

#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/utility_functions.hpp>


#include "gdbind/PackedByteArraySurface.hpp"
#include "gdbind/CPUSurfaceShader.hpp"
#include "gdbind/UltralightSingleton.hpp"
#include "gdbind/debug.hpp"
#include "gdbind/detail/JavaScriptBridge.hpp"
#include "gdbind/events/KeyEvent.hpp"
#include "gdbind/events/MouseEvent.hpp"
#include "gdbind/gpu/GodotGPUDriver.hpp"

using namespace ultralight;
using namespace godot;

namespace gdbind {
class UltralightView : public TextureRect {
    GDCLASS(UltralightView, TextureRect);

  public:
    UltralightView() : TextureRect() {
        connect("resized", Callable(this, "on_resized"));
        RenderingServer::get_singleton()->connect("frame_post_draw", Callable(this, "on_update_frame"));
        set_material(gdbind::CPUSurfaceShader::getMaterial());
        set_expand_mode(ExpandMode::EXPAND_IGNORE_SIZE);
    }

    virtual ~UltralightView() override = default;

    /// @brief Render this view to their godot texture
    ///
    /// You should call this once per frame (usually in synchrony with the monitor's refresh rate).
    /// Must call after render.updateFrame()
    void updateFrame() {
        if (view.get() == nullptr)
            return;
        if (using_accelerated) {
            auto &driver = GodotGPUDriver::instance();
            if (!driver.available()) {
                UtilityFunctions::push_warning("Ultralight GPU failed; recreating View on CPU.");
                force_cpu = true;
                initView();
                return;
            }
            auto target = view->render_target();
            auto display = driver.display_texture(target.texture_id);
            if (display.is_valid()) {
                if (texture != display) { texture = display; set_texture(texture); }
                const auto &uv = target.uv_coords;
                gpu_material->set_shader_parameter("uv_rect", Vector4(uv.left,uv.top,uv.right,uv.bottom));
            }
            return;
        }
        PackedByteArraySurface *surface = (PackedByteArraySurface *)(view->surface());
        if (!surface->dirty_bounds().IsEmpty()) {
            updateTexture(surface);
            surface->ClearDirtyBounds();
        }
    }

    void _notification(int p_what) {
        if (p_what == NOTIFICATION_PREDELETE) {
            // CanvasItem APIs remain valid here, before the engine destroys
            // its base classes and releases the GDExtension instance.
            disconnect("resized", Callable(this, "on_resized"));
            RenderingServer::get_singleton()->disconnect("frame_post_draw", Callable(this, "on_update_frame"));
            resetView();
        }
        if (p_what == NOTIFICATION_EDITOR_PRE_SAVE) {
            // To avoid meaningless storage, clear the texture before saving.
            set_texture(nullptr);
        }
        if (p_what == NOTIFICATION_EDITOR_POST_SAVE) {
            // Restore the texture for better user experience.
            set_texture(texture);
        }
    }

#pragma region godot texture property
  protected:
    Ref<Texture2D> texture;
    Ref<ShaderMaterial> gpu_material;
    bool accelerated = false, using_accelerated = false, force_cpu = false;
    uint32_t max_render_fps = 0;
    uint64_t cpu_upload_bytes = 0, cpu_mipmap_count = 0;
    double cpu_upload_us = 0;
    std::shared_ptr<UltralightRenderer::Listeners> listeners;
    std::shared_ptr<BridgeContext> bridge;
    void resetView() {
        set_texture(nullptr);
        set_material(nullptr);
        gpu_material.unref();
        texture.unref(); image.unref();
        if (bridge) { bridge->invalidate(); bridge.reset(); }
        if (view.get()) {
            view->set_load_listener(nullptr);
            view->set_view_listener(nullptr);
            view->set_network_listener(nullptr);
            view->set_download_listener(nullptr);
            view = nullptr;
        }
        listeners.reset();
        using_accelerated = false;
    }
    void setAccelerated(bool enabled) { accelerated = enabled; force_cpu = false; }
    bool getAccelerated() const { return accelerated; }
    bool isAccelerated() const { return view.get() != nullptr && using_accelerated; }
    void setMaxRenderFps(int64_t fps) {
        if (fps < 0 || uint64_t(fps) > std::numeric_limits<uint32_t>::max()) {
            UtilityFunctions::push_error("Ultralight max_render_fps must be a nonnegative 32-bit integer.");
            return;
        }
        max_render_fps = uint32_t(fps);
        if (view.get()) view->set_max_render_fps(max_render_fps);
    }
    int64_t getMaxRenderFps() const { return max_render_fps; }
    Dictionary getRenderStatistics() const {
        Dictionary result;
        const bool gpu_active = isAccelerated();
        result["accelerated"] = gpu_active;
        result["sdk_edition"] = ULTRALIGHT_EDITION_NAME;
        result["sdk_max_fps"] = int64_t(ULTRALIGHT_EDITION_MAX_FPS);
        result["max_render_fps"] = int64_t(max_render_fps);
        result["ultralight_render_us"] = UltralightRenderer::render_time_us();
        result["cpu_page_upload_bytes"] = int64_t(cpu_upload_bytes);
        result["cpu_mipmap_count"] = int64_t(cpu_mipmap_count);
        result["cpu_upload_us"] = cpu_upload_us;
        // GPUDriver totals are an optional addition for an active GPU View.
        // CPU, uninitialized and fallback Views only report the base statistics.
        if (gpu_active)
            result.merge(GodotGPUDriver::instance().statistics());
        return result;
    }
    Ref<Image> image;

    RefPtr<View> view;

    void updateTexture(PackedByteArraySurface *surface) {
        auto start = std::chrono::steady_clock::now();
        if (image == nullptr) {
            image =
                Image::create_from_data(surface->width(), surface->height(), false, Image::FORMAT_RGBA8, surface->data);
            image->set_local_to_scene(false);
            image->generate_mipmaps();
            texture = ImageTexture::create_from_image(image);
            set_texture(texture);
        } else {
            image->set_data(surface->width(), surface->height(), false, Image::FORMAT_RGBA8, surface->data);
            image->generate_mipmaps();
            Ref<ImageTexture> cpu_texture = texture;
            cpu_texture->update(image);
        }
        cpu_upload_bytes += image->get_data().size();
        ++cpu_mipmap_count;
        cpu_upload_us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now()-start).count();
    }

    void initView() {
        if (view.get() != nullptr) {
            UtilityFunctions::push_warning("view is already created, will destroy previous view.");
        }
        resetView();
        auto size = get_size();
        ViewConfig cfg;
        cfg.max_render_fps = max_render_fps;
        using_accelerated = accelerated && !force_cpu && GodotGPUDriver::instance().initialize();
        if (accelerated && !using_accelerated)
            UtilityFunctions::push_warning("Ultralight: RenderingDevice unavailable; using CPU Surface.");
        cfg.is_accelerated = using_accelerated;
        if (using_accelerated) {
            gpu_material = GodotGPUDriver::createDisplayMaterial();
            set_material(gpu_material);
        } else {
            gpu_material.unref();
            set_material(gdbind::CPUSurfaceShader::getMaterial());
        }
        cfg.is_transparent = is_transparent;
        if (size.width > 0 && size.height > 0) {
            auto result = UltralightSingleton::get_singleton()->createView(size.width, size.height, cfg, nullptr);
            view = result.view;
            listeners = result.listeners;
            result.listeners->load.onBeginLoading = [this](ultralight::View *, uint64_t, bool main, const ultralight::String &) {
                if (main && bridge) { bridge->invalidate(); bridge.reset(); }
            };
            result.listeners->load.onWindowObjectReady = [this](ultralight::View *caller, uint64_t frame_id,
                                                              bool is_main_frame, const ultralight::String &url) {
                if (!is_main_frame) return;
                {
                    if (bridge) bridge->invalidate();
                    auto lock = caller->LockJSContext();
                    bridge = BridgeContext::create(lock->ctx(), caller);
                    bindObject("__godot_view_instance", this);
                }
                emit_signal("on_window_object_ready");
            };
            result.listeners->load.onDOMReady = [this](ultralight::View *caller, uint64_t frame_id, bool is_main_frame,
                                                     const ultralight::String &url) { emit_signal("on_dom_ready"); };
            // Register bindings before loading: page scripts may use globals immediately.
            if (!html.is_empty()) {
                auto utf32 = html.utf32();
                ultralight::String32 content(utf32.get_data(), utf32.length());
                view->LoadHTML(content);
            } else if (!htmlUrl.is_empty()) {
                auto utf32 = htmlUrl.utf32();
                ultralight::String32 url(utf32.get_data(), utf32.length());
                view->LoadURL(url);
            }
        }
    }

#pragma endregion

#pragma region godot input handler
  public:
    virtual void _gui_input(const Ref<InputEvent> &p_event) override {
        if (view.get() == nullptr)
            return;
        if (auto mouse_button = dynamic_cast<InputEventMouseButton *>(p_event.ptr()); mouse_button != nullptr) {
            fireMouseEvent(mouse_button->get_position(), mouse_button->get_button_index(), mouse_button->is_pressed());
            accept_event();
        } else if (auto mouse_motion = dynamic_cast<InputEventMouseMotion *>(p_event.ptr()); mouse_motion != nullptr) {
            fireMouseEvent(mouse_motion->get_position(), MouseButton::MOUSE_BUTTON_NONE, mouse_motion->is_pressed());
            accept_event();
        }
    }

    virtual void _unhandled_input(const Ref<InputEvent> &p_event) override {
        if (view.get() == nullptr)
            return;
        if (auto key = dynamic_cast<InputEventKey *>(p_event.ptr()); view->HasFocus() && key != nullptr) {
            fireKeyEvent(key);
        } else if (auto mouse_motion = dynamic_cast<InputEventMouseMotion *>(p_event.ptr()); mouse_motion != nullptr) {
            fireMouseEvent(mouse_motion->get_position(), MouseButton::MOUSE_BUTTON_NONE, mouse_motion->is_pressed());
            accept_event();
        }
    }

  protected:
    void fireMouseEvent(godot::Vector2 position, MouseButton button_index, bool is_pressed) {
        if (button_index == MOUSE_BUTTON_WHEEL_UP || button_index == MOUSE_BUTTON_WHEEL_DOWN ||
            button_index == MOUSE_BUTTON_WHEEL_LEFT || button_index == MOUSE_BUTTON_WHEEL_RIGHT) {
            ScrollEvent evt = events::convertScrollEvent(button_index);
            view->FireScrollEvent(evt);
        } else {
            MouseEvent evt = events::convertMouseEvent(position, button_index, is_pressed);
            view->FireMouseEvent(evt);
        }
    }

    void fireKeyEvent(InputEventKey *key) {
        KeyEvent evt = events::convertInputEventKey(key);
        // You'll need to generate a key identifier from the virtual key code
        // when synthesizing events. This function is provided in KeyEvent.h
        ultralight::GetKeyIdentifierFromVirtualKeyCode(evt.virtual_key_code, evt.key_identifier);
        view->FireKeyEvent(evt);
    }

#pragma endregion

#pragma region default godot ready action
  public:
    bool lazy = false;
    void setLazy(bool lazy) {
        this->lazy = lazy;
    }

    bool getLazy() {
        return this->lazy;
    }

    // init view if not lazy
    virtual void _ready() override {
        if (view.get() != nullptr)
            return;
        if (!lazy)
            initView();
    }
#pragma endregion

#pragma region default godot signal ballback
  public:
    virtual void onResized() {
        auto size = get_size();
        set_texture(nullptr); texture.unref(); image.unref();
        if (view.get() != nullptr && size.width > 0 && size.height > 0) {
            view->Resize(size.width, size.height);
            // The next renderer frame updates the Surface or RenderTarget;
            // frame_post_draw then publishes it through this View's updateFrame().
        }
    }
#pragma endregion

#pragma region ultralight method
  protected:
    std::shared_ptr<BridgeContext> currentBridge(JSContextRef context) {
        if (!bridge || !bridge->matches(context)) {
            if (bridge) bridge->invalidate();
            bridge = BridgeContext::create(context, view.get());
        }
        return bridge;
    }
    void bindFunc(godot::String funcName, Callable callback) {
        if (!view.get()) return;
        try { auto lock = view->LockJSContext(); currentBridge(lock->ctx())->bind(funcName, callback); }
        catch (const std::exception &error) { UtilityFunctions::push_error(error.what()); }
    }
    void bindObject(godot::String propertyName, godot::Object *instance) {
        if (!view.get()) return;
        try { auto lock = view->LockJSContext(); currentBridge(lock->ctx())->bind(propertyName, instance); }
        catch (const std::exception &error) { UtilityFunctions::push_error(error.what()); }
    }
    godot::Variant executeScript(godot::String script) {
        if (!view.get()) return Variant();
        try { auto lock = view->LockJSContext(); return currentBridge(lock->ctx())->evaluate(script); }
        catch (const std::exception &error) { UtilityFunctions::push_error(error.what()); }
        return Variant();
    }

#pragma endregion

#pragma region ultralight property
  protected:
    bool is_transparent = false;
    void setTransparent(bool is_transparent) {
        this->is_transparent = is_transparent;
    }

    bool getTransparent() {
        return this->is_transparent;
    }

    /// @brief html content to load
    godot::String html;
    void setHtmlContent(godot::String html) {
        this->html = html;
        if (view.get() == nullptr)
            return;
        auto utf32 = html.utf32();
        ultralight::String32 content(utf32.get_data(), utf32.length());
        view->LoadHTML(content);
    }
    auto getHtmlContent() {
        return html;
    }

    /// @brief html url to load
    godot::String htmlUrl;
    void setHtmlUrl(godot::String htmlUrl) {
        this->htmlUrl = htmlUrl;
        if (view.get() == nullptr)
            return;
        if (htmlUrl.is_empty())
            return;
        auto utf32 = htmlUrl.utf32();
        ultralight::String32 url(utf32.get_data(), utf32.length());
        view->LoadURL(url);
    }
    auto getHtmlUrl() {
        return htmlUrl;
    }

#pragma endregion

    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("set_accelerated", "enabled"), &UltralightView::setAccelerated);
        ClassDB::bind_method(D_METHOD("get_accelerated"), &UltralightView::getAccelerated);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "accelerated"), "set_accelerated", "get_accelerated");
        ClassDB::bind_method(D_METHOD("is_accelerated"), &UltralightView::isAccelerated);
        ClassDB::bind_method(D_METHOD("get_render_statistics"), &UltralightView::getRenderStatistics);
        ClassDB::bind_method(D_METHOD("set_max_render_fps", "fps"), &UltralightView::setMaxRenderFps);
        ClassDB::bind_method(D_METHOD("get_max_render_fps"), &UltralightView::getMaxRenderFps);
        ADD_PROPERTY(PropertyInfo(Variant::INT, "max_render_fps", PROPERTY_HINT_RANGE, "0,1000,1,or_greater"), "set_max_render_fps", "get_max_render_fps");
        // ultralight region function
        ClassDB::bind_method(D_METHOD("set_html_content", "html"), &UltralightView::setHtmlContent);
        ClassDB::bind_method(D_METHOD("get_html_content"), &UltralightView::getHtmlContent);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "html"), "set_html_content", "get_html_content");

        ClassDB::bind_method(D_METHOD("set_html_url", "htmlUrl"), &UltralightView::setHtmlUrl);
        ClassDB::bind_method(D_METHOD("get_html_url"), &UltralightView::getHtmlUrl);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "htmlUrl"), "set_html_url", "get_html_url");

        ClassDB::bind_method(D_METHOD("set_transparent", "is_transparent"), &UltralightView::setTransparent);
        ClassDB::bind_method(D_METHOD("get_transparent"), &UltralightView::getTransparent);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "transparent"), "set_transparent", "get_transparent");

        ClassDB::bind_method(D_METHOD("set_lazy", "is_lazy"), &UltralightView::setLazy);
        ClassDB::bind_method(D_METHOD("get_lazy"), &UltralightView::getLazy);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lazy"), "set_lazy", "get_lazy");

        ClassDB::bind_method(D_METHOD("init_view"), &UltralightView::initView);
        ClassDB::bind_method(D_METHOD("bind_func", "funcName", "callback"), &UltralightView::bindFunc);
        ClassDB::bind_method(D_METHOD("bind_object", "propertyName", "instance"), &UltralightView::bindObject);
        ClassDB::bind_method(D_METHOD("execute_script", "script"), &UltralightView::executeScript);

        // control
        ClassDB::bind_method(D_METHOD("fire_key_event", "key_event"), &UltralightView::fireKeyEvent);
        ClassDB::bind_method(D_METHOD("fire_mouse_event", "position", "button_index", "is_pressed"),
                             &UltralightView::fireMouseEvent);

        // signal callback
        ClassDB::bind_method(D_METHOD("on_update_frame"), &UltralightView::updateFrame);
        ClassDB::bind_method(D_METHOD("on_resized"), &UltralightView::onResized);

        ADD_SIGNAL(MethodInfo("on_window_object_ready"));
        ADD_SIGNAL(MethodInfo("on_dom_ready"));
    }
};
} // namespace gdbind

#pragma once
#include <memory>

#include <Ultralight/Ultralight.h>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/texture_rect.hpp>

#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <ulbind17/ulbind17.hpp>

#include "gdbind/PackedByteArraySurface.hpp"
#include "gdbind/Shader.hpp"
#include "gdbind/UltralightSingleton.hpp"
#include "gdbind/debug.hpp"
#include "gdbind/detail/cast.hpp"
#include "gdbind/detail/godot_callable.hpp"
#include "gdbind/detail/godot_object.hpp"

using namespace ultralight;
using namespace godot;

namespace gdbind {
class UltralightView : public TextureRect {
    GDCLASS(UltralightView, TextureRect);

  public:
    UltralightView() : TextureRect() {
        connect("resized", Callable(this, "on_resized"));
        RenderingServer::get_singleton()->connect("frame_post_draw", Callable(this, "on_update_frame"));
        set_material(gdbind::Shader::bulitin_material());
        set_expand_mode(ExpandMode::EXPAND_IGNORE_SIZE);
    }

    virtual ~UltralightView() override {
        disconnect("resized", Callable(this, "on_resized"));
        RenderingServer::get_singleton()->disconnect("frame_post_draw", Callable(this, "on_update_frame"));

        view = nullptr;
    }

    /// @brief Render this view to their godot texture
    ///
    /// You should call this once per frame (usually in synchrony with the monitor's refresh rate).
    /// Must call after render.updateFrame()
    void updateFrame() {
        if (view.get() == nullptr)
            return;
        PackedByteArraySurface *surface = (PackedByteArraySurface *)(view->surface());
        if (!surface->dirty_bounds().IsEmpty()) {
            updateTexture(surface);
            surface->ClearDirtyBounds();
        }
    }

#pragma region godot texture property
  protected:
    Ref<ImageTexture> texture;
    Ref<Image> image;

    RefPtr<View> view;

    void updateTexture(PackedByteArraySurface *surface) {
        if (image == nullptr) {
            image =
                Image::create_from_data(surface->width(), surface->height(), false, Image::FORMAT_RGBA8, surface->data);
            image->generate_mipmaps();
            texture = ImageTexture::create_from_image(image);
            set_texture(texture);
        } else {
            image->set_data(surface->width(), surface->height(), false, Image::FORMAT_RGBA8, surface->data);
            image->generate_mipmaps();
            texture->update(image);
        }
    }

    void initView() {
        if (view.get() != nullptr) {
            UtilityFunctions::push_warning("view is already created, will destroy previous view.");
        }
        auto size = get_size();
        ViewConfig cfg;
        cfg.is_accelerated = false;
        cfg.is_transparent = is_transparent;
        if (size.width > 0 && size.height > 0) {
            auto result = UltralightSingleton::get_singleton()->createView(size.width, size.height, cfg, nullptr);
            view = result.view;
            if (!html.is_empty()) {
                auto utf32 = html.utf32();
                ultralight::String32 content(utf32.get_data(), utf32.length());
                view->LoadHTML(content);
            } else if (!htmlUrl.is_empty()) {
                auto utf32 = htmlUrl.utf32();
                ultralight::String32 url(utf32.get_data(), utf32.length());
                view->LoadURL(url);
            }

            result.loadListener->onWindowObjectReady = [this](ultralight::View *caller, uint64_t frame_id,
                                                              bool is_main_frame, const ultralight::String &url) {
                {
                    gdbind::GodotObject::defindJSClass(view->LockJSContext()->ctx());
                    bindObject("__godot_view_instance", this);
                }
                emit_signal("on_window_object_ready");
            };
            result.loadListener->onDOMReady = [this](ultralight::View *caller, uint64_t frame_id, bool is_main_frame,
                                                     const ultralight::String &url) { emit_signal("on_dom_ready"); };
        }
    }

#pragma endregion

#pragma region godot input handler
  public:
    virtual void _gui_input(const Ref<InputEvent> &p_event) {
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

  protected:
    void fireMouseEvent(godot::Vector2 position, MouseButton button_index, bool is_pressed) {
        MouseEvent evt;
        evt.type = is_pressed ? MouseEvent::kType_MouseDown : MouseEvent::kType_MouseUp;
        evt.x = position.x;
        evt.y = position.y;
        switch (button_index) {
        case MouseButton::MOUSE_BUTTON_LEFT:
            evt.button = MouseEvent::kButton_Left;
            break;
        case MouseButton::MOUSE_BUTTON_RIGHT:
            evt.button = MouseEvent::kButton_Right;
            break;
        case MouseButton::MOUSE_BUTTON_MIDDLE:
            evt.button = MouseEvent::kButton_Middle;
            break;
        default:
            evt.button = MouseEvent::kButton_None;
            evt.type = MouseEvent::kType_MouseMoved;
            break;
        }
        view->FireMouseEvent(evt);
    }
#pragma endregion

#pragma region default godot ready action
  public:
    // init view
    virtual void _ready() override {
        if (view.get() != nullptr)
            return;
        initView();
    }
#pragma endregion

#pragma region default godot signal ballback
  public:
    virtual void onResized() {
        auto size = get_size();
        image.unref();
        if (view.get() != nullptr) {
            view->Resize(size.width, size.height);
            updateFrame();
        }
    }
#pragma endregion

#pragma region ultralight method
  protected:
    void bindFunc(godot::String funcName, Callable callback) {
        auto ctx = view->LockJSContext();
        auto window = ulbind17::detail::Object::GetGlobalObject(ctx->ctx());
        window.set(funcName.utf8().ptr(), std::move(Variant(callback)));
    }

    void bindObject(godot::String propertyName, godot::Object *instance) {
        auto ctx = view->LockJSContext();
        auto window = ulbind17::detail::Object::GetGlobalObject(ctx->ctx());
        window.set(propertyName.utf8().ptr(), std::move(Variant(instance)));
    }

    godot::Variant executeScript(godot::String script) {
        auto ctx = view->LockJSContext();
        return ulbind17::detail::Script(ctx->ctx(), script.utf8().ptr()).Evaluate<Variant>();
    }

#pragma endregion

#pragma region ultralight property
  protected:
    bool is_transparent;
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

        ClassDB::bind_method(D_METHOD("init_view"), &UltralightView::initView);
        ClassDB::bind_method(D_METHOD("bind_func", "funcName", "callback"), &UltralightView::bindFunc);
        ClassDB::bind_method(D_METHOD("bind_object", "propertyName", "instance"), &UltralightView::bindObject);
        ClassDB::bind_method(D_METHOD("execute_script", "script"), &UltralightView::executeScript);

        // signal callback
        ClassDB::bind_method(D_METHOD("on_update_frame"), &UltralightView::updateFrame);
        ClassDB::bind_method(D_METHOD("on_resized"), &UltralightView::onResized);

        ADD_SIGNAL(MethodInfo("on_window_object_ready"));
        ADD_SIGNAL(MethodInfo("on_dom_ready"));
    }
};
} // namespace gdbind

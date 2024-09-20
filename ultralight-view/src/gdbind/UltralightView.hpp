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
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/texture_rect.hpp>

#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <ulbind17/ulbind17.hpp>

#include "PackedByteArraySurface.hpp"
#include "UltralightSingleton.hpp"
#include "debug.hpp"
#include "detail/cast.hpp"
#include "detail/godot_callable.hpp"

using namespace ultralight;
using namespace godot;

namespace gdbind {
class UltralightView : public TextureRect {
    GDCLASS(UltralightView, TextureRect);

  public:
    /// @brief Render this view to their godot texture
    ///
    /// You should call this once per frame (usually in synchrony with the monitor's refresh rate).
    /// Must call after render.updateFrame()
    void updateFrame() {
        PackedByteArraySurface *surface = (PackedByteArraySurface *)(view->surface());
        if (!surface->dirty_bounds().IsEmpty()) {
            updateTexture(surface);
            surface->ClearDirtyBounds();
        }
    }

    UltralightView() : TextureRect() {
        connect("resized", Callable(this, "on_resized"));
        RenderingServer::get_singleton()->connect("frame_post_draw", Callable(this, "on_update_frame"));
        set_material(bulitin_material());
    }

    ~UltralightView() {
        disconnect("resized", Callable(this, "on_resized"));
        RenderingServer::get_singleton()->disconnect("frame_post_draw", Callable(this, "on_update_frame"));
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
            texture = ImageTexture::create_from_image(image);
            set_texture(texture);
        } else {
            image->set_data(surface->width(), surface->height(), false, Image::FORMAT_RGBA8, surface->data);
            texture->update(image);
        }
    }
#pragma endregion

#pragma region godot input handler
  public:
    virtual void _gui_input(const Ref<InputEvent> &p_event) {
        if (view.get() == nullptr)
            return;
        if (auto mouse_button = dynamic_cast<InputEventMouseButton *>(p_event.ptr()); mouse_button != nullptr) {
            fire_mouse_event(mouse_button);
            accept_event();
        } else if (auto mouse_motion = dynamic_cast<InputEventMouseMotion *>(p_event.ptr()); mouse_motion != nullptr) {
            fire_mouse_event(mouse_motion);
            accept_event();
        }
    }

  protected:
    void fire_mouse_event(InputEventMouseButton *mouse_button) {
        MouseEvent evt;
        evt.type = mouse_button->is_pressed() ? MouseEvent::kType_MouseDown : MouseEvent::kType_MouseUp;
        evt.x = mouse_button->get_position().x;
        evt.y = mouse_button->get_position().y;
        switch (mouse_button->get_button_index()) {
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
            break;
        }
        view->FireMouseEvent(evt);
    }

    void fire_mouse_event(InputEventMouseMotion *mouse_motion) {
        MouseEvent evt;
        evt.type = MouseEvent::kType_MouseMoved;
        evt.x = mouse_motion->get_position().x;
        evt.y = mouse_motion->get_position().y;
        evt.button = MouseEvent::kButton_None;
        view->FireMouseEvent(evt);
    }
#pragma endregion

#pragma region default godot perframe action
  public:
    // update per ticket
    virtual void _process(double p_delta) {
        if (view.get() == nullptr)
            return;

        auto ultralight = UltralightSingleton::get_singleton();
        ultralight->updateLogic();
    }
#pragma endregion

#pragma region default godot ready action
  public:
    // init view
    virtual void _ready() override {
        if (view.get() != nullptr)
            return;

        auto renderer = UltralightSingleton::get_singleton()->getRender();
        ViewConfig cfg;
        cfg.is_accelerated = false;
        auto size = get_size();
        if (size.width > 0 && size.height > 0) {
            view = renderer->createView(size.width, size.height, cfg, nullptr);
            if (!html.is_empty()) {
                auto utf32 = html.utf32();
                ultralight::String32 content(utf32.get_data(), utf32.length());
                view->LoadHTML(content);
            } else if (htmlFile.is_valid()) {
                auto utf32 = FileAccess::get_file_as_string(htmlFile->get_path()).utf32();
                ultralight::String32 content(utf32.get_data(), utf32.length());
                view->LoadHTML(content);
            }
        }
    }
#pragma endregion

#pragma region default godot signal ballback
  public:
    virtual void onResized() {
        auto size = get_size();
        image.unref();
        view->Resize(size.width, size.height);
        updateFrame();
    }
#pragma endregion

#pragma region shader use to convert BGRA to RGBA
  protected:
    static ShaderMaterial *bulitin_material() {
        static ShaderMaterial *material;
        if (material == nullptr) {
            material = memnew(ShaderMaterial);
            static Shader *shader = memnew(Shader);
            // convert BGRA(ultralight format) to RGBA(godot format)
            shader->set_code(R"(
shader_type canvas_item;

void fragment(){
  float b = COLOR.r;
  COLOR.r = COLOR.b;
  COLOR.b = b;
}
            )");
            material->set_shader(shader);
        }
        return material;
    }
#pragma endregion

#pragma region ultralight method
  protected:
    void bindFunc(godot::String funcName, Callable callback) {
        auto ctx = view->LockJSContext();
        auto window = ulbind17::detail::Object::GetGlobalObject(ctx->ctx());
        window.set(funcName.utf8().ptr(), std::move(Variant(callback)));
    }

    auto executeScript(godot::String script) {
        auto ctx = view->LockJSContext();
        return ulbind17::detail::Script(ctx->ctx(), script.utf8().ptr()).Evaluate<Variant>();
    }

#pragma endregion

#pragma region ultralight property
  protected:
    /// @brief html content to load
    godot::String html;
    void setHtmlContent(godot::String html) {
        this->html = html;
        this->htmlFile.unref();
        if (view.get() == nullptr)
            return;
        auto utf32 = html.utf32();
        ultralight::String32 content(utf32.get_data(), utf32.length());
        view->LoadHTML(content);
    }
    auto getHtmlContent() {
        return html;
    }
    /// @brief html file to load
    Ref<godot::Resource> htmlFile;
    void setHtmlFile(Ref<godot::Resource> htmlFile) {
        this->htmlFile = htmlFile;
        this->html = "";
        if (view.get() == nullptr)
            return;

        auto utf32 = FileAccess::get_file_as_string(htmlFile->get_path()).utf32();
        ultralight::String32 content(utf32.get_data(), utf32.length());
        view->LoadHTML(content);
    }
    auto getHtmlFile() {
        return htmlFile;
    }

#pragma endregion

    static void _bind_methods() {
        // ultralight region function
        ClassDB::bind_method(D_METHOD("set_html_content", "html"), &UltralightView::setHtmlContent);
        ClassDB::bind_method(D_METHOD("get_html_content"), &UltralightView::getHtmlContent);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "html"), "set_html_content", "get_html_content");

        ClassDB::bind_method(D_METHOD("set_html_file", "htmlFile"), &UltralightView::setHtmlFile);
        ClassDB::bind_method(D_METHOD("get_html_file"), &UltralightView::getHtmlFile);
        ADD_PROPERTY(PropertyInfo(Variant::RID, "htmlFile"), "set_html_file", "get_html_file");

        ClassDB::bind_method(D_METHOD("bind_func", "funcName", "callback"), &UltralightView::bindFunc);
        ClassDB::bind_method(D_METHOD("execute_script", "script"), &UltralightView::executeScript);

        // signal callback
        ClassDB::bind_method(D_METHOD("on_update_frame"), &UltralightView::updateFrame);
        ClassDB::bind_method(D_METHOD("on_resized"), &UltralightView::onResized);
    }
};
} // namespace gdbind

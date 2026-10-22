#pragma once
#include "UltralightView.hpp"
namespace gdbind {
class UltralightInspector : public UltralightView {
    GDCLASS(UltralightInspector, UltralightView);
    // Track a Node without keeping it alive; resolve again after every rebuild.
    godot::ObjectID target_id;
    ultralight::RefPtr<ultralight::View> target_native;
    ultralight::RefPtr<ultralight::View> inspector;

  public:
    UltralightInspector() {
        lazy = true;
    }
    void set_target_view(UltralightView *target) {
        target_id = target ? godot::ObjectID(target->get_instance_id()) : godot::ObjectID();
        resetView();
        target_native = nullptr;
        inspector = nullptr;
    }
    UltralightView *get_target_view() const {
        return godot::Object::cast_to<UltralightView>(godot::ObjectDB::get_instance(target_id));
    }
    void _process(double) override {
        auto *target = get_target_view();
        auto native = target ? target->nativeView() : nullptr;
        if (target_native.get() != native.get()) {
            resetView();
            target_native = native;
            inspector = nullptr;
        }
        if (!is_visible_in_tree() || !native.get())
            return;
        if (!inspector.get()) {
            if (!target->inspectorView().get())
                native->CreateLocalInspectorView();
            inspector = target->inspectorView();
            if (!inspector.get())
                return;
            view = inspector;
            using_accelerated = false;
            listeners = target->inspectorListeners();
            listeners->view.onCursor = [this](ultralight::Cursor cursor) {
                set_default_cursor_shape(cursorShape(cursor));
            };
            view->set_view_listener(&listeners->view);
            set_material(gdbind::CPUSurfaceShader::getMaterial());
            view->Resize(MAX(1, get_size().x), MAX(1, get_size().y));
        }
    }

  protected:
    static void _bind_methods() {
        godot::ClassDB::bind_method(godot::D_METHOD("set_target_view", "view"), &UltralightInspector::set_target_view);
        godot::ClassDB::bind_method(godot::D_METHOD("get_target_view"), &UltralightInspector::get_target_view);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "target_view", godot::PROPERTY_HINT_NODE_TYPE,
                                         "UltralightView"),
                     "set_target_view", "get_target_view");
    }
};
} // namespace gdbind

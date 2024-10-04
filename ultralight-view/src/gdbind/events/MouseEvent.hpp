#pragma once

#include <Ultralight/Ultralight.h>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input_event_key.hpp>

namespace gdbind {
namespace events {
inline ultralight::MouseEvent convertMouseEvent(godot::Vector2 position, godot::MouseButton button_index,
                                                bool is_pressed) {
    using namespace ultralight;
    using namespace godot;

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
    return evt;
}

} // namespace events
} // namespace gdbind

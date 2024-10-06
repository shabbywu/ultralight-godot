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

inline ultralight::ScrollEvent convertScrollEvent(godot::MouseButton button_index) {
    using namespace ultralight;
    using namespace godot;

    ScrollEvent evt;
    evt.type = ScrollEvent::kType_ScrollByPixel;
    evt.delta_x = 0;
    evt.delta_y = 0;
    const int step = 30;
    if (button_index == MouseButton::MOUSE_BUTTON_WHEEL_UP) {
        evt.delta_y = step;
    } else if (button_index == MouseButton::MOUSE_BUTTON_WHEEL_DOWN) {
        evt.delta_y = -step;
    } else if (button_index == MouseButton::MOUSE_BUTTON_WHEEL_LEFT) {
        evt.delta_x = step;
    } else if (button_index == MouseButton::MOUSE_BUTTON_WHEEL_RIGHT) {
        evt.delta_x = -step;
    }
    return evt;
}

} // namespace events
} // namespace gdbind

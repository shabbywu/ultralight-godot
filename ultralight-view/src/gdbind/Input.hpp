#pragma once
#include <Ultralight/Ultralight.h>
#include <godot_cpp/classes/control.hpp>
namespace gdbind {
inline godot::Control::CursorShape cursorShape(ultralight::Cursor cursor) {
    using C = godot::Control;
    switch (cursor) {
    case ultralight::kCursor_IBeam:
        return C::CURSOR_IBEAM;
    case ultralight::kCursor_Hand:
        return C::CURSOR_POINTING_HAND;
    case ultralight::kCursor_Cross:
        return C::CURSOR_CROSS;
    case ultralight::kCursor_Wait:
        return C::CURSOR_WAIT;
    case ultralight::kCursor_Progress:
        return C::CURSOR_BUSY;
    case ultralight::kCursor_Grabbing:
        return C::CURSOR_DRAG;
    case ultralight::kCursor_Copy:
        return C::CURSOR_CAN_DROP;
    case ultralight::kCursor_NotAllowed:
        return C::CURSOR_FORBIDDEN;
    case ultralight::kCursor_NorthSouthResize:
        return C::CURSOR_VSIZE;
    case ultralight::kCursor_EastWestResize:
        return C::CURSOR_HSIZE;
    case ultralight::kCursor_NorthWestSouthEastResize:
        return C::CURSOR_BDIAGSIZE;
    case ultralight::kCursor_NorthEastSouthWestResize:
        return C::CURSOR_FDIAGSIZE;
    case ultralight::kCursor_Move:
        return C::CURSOR_MOVE;
    case ultralight::kCursor_RowResize:
        return C::CURSOR_VSPLIT;
    case ultralight::kCursor_ColumnResize:
        return C::CURSOR_HSPLIT;
    case ultralight::kCursor_Help:
        return C::CURSOR_HELP;
    default:
        return C::CURSOR_ARROW;
    }
}
} // namespace gdbind

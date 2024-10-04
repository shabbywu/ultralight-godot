#pragma once

#include <Ultralight/Ultralight.h>
#include <godot_cpp/classes/input_event_key.hpp>

namespace gdbind {
namespace events {
inline ultralight::KeyEvent convertInputEventKey(godot::InputEventKey *key) {
    using namespace ultralight;
    using namespace godot;

    KeyEvent evt;
    evt.type = KeyEvent::kType_RawKeyDown;
    evt.native_key_code = 0;
    // convert modifiers
    auto mask = key->get_modifiers_mask();
    if (mask & KeyModifierMask::KEY_MASK_CTRL) {
        evt.modifiers |= KeyEvent::kMod_CtrlKey;
    }
    if (mask & KeyModifierMask::KEY_MASK_ALT) {
        evt.modifiers |= KeyEvent::kMod_AltKey;
    }
    if (mask & KeyModifierMask::KEY_MASK_META) {
        evt.modifiers |= KeyEvent::kMod_MetaKey;
    }
    if (mask & KeyModifierMask::KEY_MASK_SHIFT) {
        evt.modifiers |= KeyEvent::kMod_ShiftKey;
    }

    // convert keycode
    auto keycode = key->get_keycode();
    switch (keycode) {
    case Key::KEY_NONE:
        evt.virtual_key_code = KeyCodes::GK_UNKNOWN;
        break;

        // case Key::KEY_SPECIAL:
        //     evt.virtual_key_code = KeyCodes::GK_SPECIAL;
        //     break;

    case Key::KEY_ESCAPE:
        evt.virtual_key_code = KeyCodes::GK_ESCAPE;
        break;

    case Key::KEY_TAB:
        evt.virtual_key_code = KeyCodes::GK_TAB;
        break;

    case Key::KEY_BACKTAB:
        evt.virtual_key_code = KeyCodes::GK_BACK;
        break;

    case Key::KEY_BACKSPACE:
        evt.virtual_key_code = KeyCodes::GK_BACK;
        break;

    case Key::KEY_ENTER:
        evt.virtual_key_code = KeyCodes::GK_RETURN;
        break;

    case Key::KEY_KP_ENTER:
        evt.virtual_key_code = KeyCodes::GK_RETURN;
        break;

    case Key::KEY_INSERT:
        evt.virtual_key_code = KeyCodes::GK_INSERT;
        break;

    case Key::KEY_DELETE:
        evt.virtual_key_code = KeyCodes::GK_DELETE;
        break;

    case Key::KEY_PAUSE:
        evt.virtual_key_code = KeyCodes::GK_PAUSE;
        break;

    case Key::KEY_PRINT:
        evt.virtual_key_code = KeyCodes::GK_PRINT;
        break;

        // case Key::KEY_SYSREQ:
        //     evt.virtual_key_code = KeyCodes::GK_SYSREQ;
        //     break;

    case Key::KEY_CLEAR:
        evt.virtual_key_code = KeyCodes::GK_CLEAR;
        break;

    case Key::KEY_HOME:
        evt.virtual_key_code = KeyCodes::GK_HOME;
        break;

    case Key::KEY_END:
        evt.virtual_key_code = KeyCodes::GK_END;
        break;

    case Key::KEY_LEFT:
        evt.virtual_key_code = KeyCodes::GK_LEFT;
        break;

    case Key::KEY_UP:
        evt.virtual_key_code = KeyCodes::GK_UP;
        break;

    case Key::KEY_RIGHT:
        evt.virtual_key_code = KeyCodes::GK_RIGHT;
        break;

    case Key::KEY_DOWN:
        evt.virtual_key_code = KeyCodes::GK_DOWN;
        break;

    case Key::KEY_PAGEUP:
        evt.virtual_key_code = KeyCodes::GK_PRIOR;
        break;

    case Key::KEY_PAGEDOWN:
        evt.virtual_key_code = KeyCodes::GK_NEXT;
        break;

    case Key::KEY_SHIFT:
        evt.virtual_key_code = KeyCodes::GK_SHIFT;
        break;

        // case Key::KEY_CTRL:
        //     evt.virtual_key_code = KeyCodes::GK_CTRL;
        //     break;

        // case Key::KEY_META:
        //     evt.virtual_key_code = KeyCodes::GK_META;
        //     break;

        // case Key::KEY_ALT:
        //     evt.virtual_key_code = KeyCodes::GK_ALT;
        //     break;

        // case Key::KEY_CAPSLOCK:
        //     evt.virtual_key_code = KeyCodes::GK_CAPSLOCK;
        //     break;

    case Key::KEY_NUMLOCK:
        evt.virtual_key_code = KeyCodes::GK_NUMLOCK;
        break;

    case Key::KEY_SCROLLLOCK:
        evt.virtual_key_code = KeyCodes::GK_SCROLL;
        break;

    case Key::KEY_F1:
        evt.virtual_key_code = KeyCodes::GK_F1;
        break;

    case Key::KEY_F2:
        evt.virtual_key_code = KeyCodes::GK_F2;
        break;

    case Key::KEY_F3:
        evt.virtual_key_code = KeyCodes::GK_F3;
        break;

    case Key::KEY_F4:
        evt.virtual_key_code = KeyCodes::GK_F4;
        break;

    case Key::KEY_F5:
        evt.virtual_key_code = KeyCodes::GK_F5;
        break;

    case Key::KEY_F6:
        evt.virtual_key_code = KeyCodes::GK_F6;
        break;

    case Key::KEY_F7:
        evt.virtual_key_code = KeyCodes::GK_F7;
        break;

    case Key::KEY_F8:
        evt.virtual_key_code = KeyCodes::GK_F8;
        break;

    case Key::KEY_F9:
        evt.virtual_key_code = KeyCodes::GK_F9;
        break;

    case Key::KEY_F10:
        evt.virtual_key_code = KeyCodes::GK_F10;
        break;

    case Key::KEY_F11:
        evt.virtual_key_code = KeyCodes::GK_F11;
        break;

    case Key::KEY_F12:
        evt.virtual_key_code = KeyCodes::GK_F12;
        break;

    case Key::KEY_F13:
        evt.virtual_key_code = KeyCodes::GK_F13;
        break;

    case Key::KEY_F14:
        evt.virtual_key_code = KeyCodes::GK_F14;
        break;

    case Key::KEY_F15:
        evt.virtual_key_code = KeyCodes::GK_F15;
        break;

    case Key::KEY_F16:
        evt.virtual_key_code = KeyCodes::GK_F16;
        break;

    case Key::KEY_F17:
        evt.virtual_key_code = KeyCodes::GK_F17;
        break;

    case Key::KEY_F18:
        evt.virtual_key_code = KeyCodes::GK_F18;
        break;

    case Key::KEY_F19:
        evt.virtual_key_code = KeyCodes::GK_F19;
        break;

    case Key::KEY_F20:
        evt.virtual_key_code = KeyCodes::GK_F20;
        break;

    case Key::KEY_F21:
        evt.virtual_key_code = KeyCodes::GK_F21;
        break;

    case Key::KEY_F22:
        evt.virtual_key_code = KeyCodes::GK_F22;
        break;

    case Key::KEY_F23:
        evt.virtual_key_code = KeyCodes::GK_F23;
        break;

    case Key::KEY_F24:
        evt.virtual_key_code = KeyCodes::GK_F24;
        break;

        // case Key::KEY_F25:
        //     evt.virtual_key_code = KeyCodes::GK_F25;
        //     break;

        // case Key::KEY_F26:
        //     evt.virtual_key_code = KeyCodes::GK_F26;
        //     break;

        // case Key::KEY_F27:
        //     evt.virtual_key_code = KeyCodes::GK_F27;
        //     break;

        // case Key::KEY_F28:
        //     evt.virtual_key_code = KeyCodes::GK_F28;
        //     break;

        // case Key::KEY_F29:
        //     evt.virtual_key_code = KeyCodes::GK_F29;
        //     break;

        // case Key::KEY_F30:
        //     evt.virtual_key_code = KeyCodes::GK_F30;
        //     break;

        // case Key::KEY_F31:
        //     evt.virtual_key_code = KeyCodes::GK_F31;
        //     break;

        // case Key::KEY_F32:
        //     evt.virtual_key_code = KeyCodes::GK_F32;
        //     break;

        // case Key::KEY_F33:
        //     evt.virtual_key_code = KeyCodes::GK_F33;
        //     break;

        // case Key::KEY_F34:
        //     evt.virtual_key_code = KeyCodes::GK_F34;
        //     break;

        // case Key::KEY_F35:
        //     evt.virtual_key_code = KeyCodes::GK_F35;
        //     break;

    case Key::KEY_KP_MULTIPLY:
        evt.virtual_key_code = KeyCodes::GK_MULTIPLY;
        break;

    case Key::KEY_KP_DIVIDE:
        evt.virtual_key_code = KeyCodes::GK_DIVIDE;
        break;

    case Key::KEY_KP_SUBTRACT:
        evt.virtual_key_code = KeyCodes::GK_SUBTRACT;
        break;

    case Key::KEY_KP_PERIOD:
        evt.virtual_key_code = KeyCodes::GK_OEM_PERIOD;
        break;

    case Key::KEY_KP_ADD:
        evt.virtual_key_code = KeyCodes::GK_ADD;
        break;

    case Key::KEY_KP_0:
        evt.virtual_key_code = KeyCodes::GK_0;
        break;

    case Key::KEY_KP_1:
        evt.virtual_key_code = KeyCodes::GK_1;
        break;

    case Key::KEY_KP_2:
        evt.virtual_key_code = KeyCodes::GK_2;
        break;

    case Key::KEY_KP_3:
        evt.virtual_key_code = KeyCodes::GK_3;
        break;

    case Key::KEY_KP_4:
        evt.virtual_key_code = KeyCodes::GK_4;
        break;

    case Key::KEY_KP_5:
        evt.virtual_key_code = KeyCodes::GK_5;
        break;

    case Key::KEY_KP_6:
        evt.virtual_key_code = KeyCodes::GK_6;
        break;

    case Key::KEY_KP_7:
        evt.virtual_key_code = KeyCodes::GK_7;
        break;

    case Key::KEY_KP_8:
        evt.virtual_key_code = KeyCodes::GK_8;
        break;

    case Key::KEY_KP_9:
        evt.virtual_key_code = KeyCodes::GK_9;
        break;

    case Key::KEY_MENU:
        evt.virtual_key_code = KeyCodes::GK_MENU;
        break;

        // case Key::KEY_HYPER:
        //     evt.virtual_key_code = KeyCodes::GK_HYPER;
        //     break;

    case Key::KEY_HELP:
        evt.virtual_key_code = KeyCodes::GK_HELP;
        break;

    case Key::KEY_BACK:
        evt.virtual_key_code = KeyCodes::GK_BACK;
        break;

    case Key::KEY_FORWARD:
        evt.virtual_key_code = KeyCodes::GK_BROWSER_FORWARD;
        break;

    case Key::KEY_STOP:
        evt.virtual_key_code = KeyCodes::GK_BROWSER_STOP;
        break;

    case Key::KEY_REFRESH:
        evt.virtual_key_code = KeyCodes::GK_BROWSER_REFRESH;
        break;

    case Key::KEY_VOLUMEDOWN:
        evt.virtual_key_code = KeyCodes::GK_VOLUME_DOWN;
        break;

    case Key::KEY_VOLUMEMUTE:
        evt.virtual_key_code = KeyCodes::GK_VOLUME_MUTE;
        break;

    case Key::KEY_VOLUMEUP:
        evt.virtual_key_code = KeyCodes::GK_VOLUME_UP;
        break;

    case Key::KEY_MEDIAPLAY:
        evt.virtual_key_code = KeyCodes::GK_MEDIA_PLAY_PAUSE;
        break;

    case Key::KEY_MEDIASTOP:
        evt.virtual_key_code = KeyCodes::GK_MEDIA_STOP;
        break;

    case Key::KEY_MEDIAPREVIOUS:
        evt.virtual_key_code = KeyCodes::GK_MEDIA_PREV_TRACK;
        break;

    case Key::KEY_MEDIANEXT:
        evt.virtual_key_code = KeyCodes::GK_MEDIA_NEXT_TRACK;
        break;

        // case Key::KEY_MEDIARECORD:
        //     evt.virtual_key_code = KeyCodes::GK_MEDIARECORD;
        //     break;

    case Key::KEY_HOMEPAGE:
        evt.virtual_key_code = KeyCodes::GK_HOME;
        break;

    case Key::KEY_FAVORITES:
        evt.virtual_key_code = KeyCodes::GK_BROWSER_FAVORITES;
        break;

    case Key::KEY_SEARCH:
        evt.virtual_key_code = KeyCodes::GK_BROWSER_SEARCH;
        break;

        // case Key::KEY_STANDBY:
        //     evt.virtual_key_code = KeyCodes::GK_STANDBY;
        //     break;

        // case Key::KEY_OPENURL:
        //     evt.virtual_key_code = KeyCodes::GK_OPENURL;
        //     break;

    case Key::KEY_LAUNCHMAIL:
        evt.virtual_key_code = KeyCodes::GK_MEDIA_LAUNCH_MAIL;
        break;

    case Key::KEY_LAUNCHMEDIA:
        evt.virtual_key_code = KeyCodes::GK_MEDIA_LAUNCH_MEDIA_SELECT;
        break;

        // case Key::KEY_LAUNCH0:
        //     evt.virtual_key_code = KeyCodes::GK_LAUNCH0;
        //     break;

        // case Key::KEY_LAUNCH1:
        //     evt.virtual_key_code = KeyCodes::GK_LAUNCH1;
        //     break;

        // case Key::KEY_LAUNCH2:
        //     evt.virtual_key_code = KeyCodes::GK_LAUNCH2;
        //     break;

        // case Key::KEY_LAUNCH3:
        //     evt.virtual_key_code = KeyCodes::GK_LAUNCH3;
        //     break;

        // case Key::KEY_LAUNCH4:
        //     evt.virtual_key_code = KeyCodes::GK_LAUNCH4;
        //     break;

        // case Key::KEY_LAUNCH5:
        //     evt.virtual_key_code = KeyCodes::GK_LAUNCH5;
        //     break;

        // case Key::KEY_LAUNCH6:
        //     evt.virtual_key_code = KeyCodes::GK_LAUNCH6;
        //     break;

        // case Key::KEY_LAUNCH7:
        //     evt.virtual_key_code = KeyCodes::GK_LAUNCH7;
        //     break;

        // case Key::KEY_LAUNCH8:
        //     evt.virtual_key_code = KeyCodes::GK_LAUNCH8;
        //     break;

        // case Key::KEY_LAUNCH9:
        //     evt.virtual_key_code = KeyCodes::GK_LAUNCH9;
        //     break;

        // case Key::KEY_LAUNCHA:
        //     evt.virtual_key_code = KeyCodes::GK_LAUNCHA;
        //     break;

        // case Key::KEY_LAUNCHB:
        //     evt.virtual_key_code = KeyCodes::GK_LAUNCHB;
        //     break;

        // case Key::KEY_LAUNCHC:
        //     evt.virtual_key_code = KeyCodes::GK_LAUNCHC;
        //     break;

        // case Key::KEY_LAUNCHD:
        //     evt.virtual_key_code = KeyCodes::GK_LAUNCHD;
        //     break;

        // case Key::KEY_LAUNCHE:
        //     evt.virtual_key_code = KeyCodes::GK_LAUNCHE;
        //     break;

        // case Key::KEY_LAUNCHF:
        //     evt.virtual_key_code = KeyCodes::GK_LAUNCHF;
        //     break;

        // case Key::KEY_GLOBE:
        //     evt.virtual_key_code = KeyCodes::GK_GLOBE;
        //     break;

        // case Key::KEY_KEYBOARD:
        //     evt.virtual_key_code = KeyCodes::GK_KEYBOARD;
        //     break;

        // case Key::KEY_JIS_EISU:
        //     evt.virtual_key_code = KeyCodes::GK_JIS_EISU;
        //     break;

        // case Key::KEY_JIS_KANA:
        //     evt.virtual_key_code = KeyCodes::GK_JIS_KANA;
        //     break;

    case Key::KEY_UNKNOWN:
        evt.virtual_key_code = KeyCodes::GK_UNKNOWN;
        break;

    case Key::KEY_SPACE:
        evt.virtual_key_code = KeyCodes::GK_SPACE;
        break;

        // case Key::KEY_EXCLAM:
        //     evt.virtual_key_code = KeyCodes::GK_EXCLAM;
        //     break;

        // case Key::KEY_QUOTEDBL:
        //     evt.virtual_key_code = KeyCodes::GK_QUOTEDBL;
        //     break;

        // case Key::KEY_NUMBERSIGN:
        //     evt.virtual_key_code = KeyCodes::GK_NUMBERSIGN;
        //     break;

        // case Key::KEY_DOLLAR:
        //     evt.virtual_key_code = KeyCodes::GK_DOLLAR;
        //     break;

        // case Key::KEY_PERCENT:
        //     evt.virtual_key_code = KeyCodes::GK_PERCENT;
        //     break;

        // case Key::KEY_AMPERSAND:
        //     evt.virtual_key_code = KeyCodes::GK_AMPERSAND;
        //     break;

        // case Key::KEY_APOSTROPHE:
        //     evt.virtual_key_code = KeyCodes::GK_APOSTROPHE;
        //     break;

        // case Key::KEY_PARENLEFT:
        //     evt.virtual_key_code = KeyCodes::GK_PARENLEFT;
        //     break;

        // case Key::KEY_PARENRIGHT:
        //     evt.virtual_key_code = KeyCodes::GK_PARENRIGHT;
        //     break;

        // case Key::KEY_ASTERISK:
        //     evt.virtual_key_code = KeyCodes::GK_ASTERISK;
        //     break;

        // case Key::KEY_PLUS:
        //     evt.virtual_key_code = KeyCodes::GK_PLUS;
        //     break;

        // case Key::KEY_COMMA:
        //     evt.virtual_key_code = KeyCodes::GK_COMMA;
        //     break;

        // case Key::KEY_MINUS:
        //     evt.virtual_key_code = KeyCodes::GK_MINUS;
        //     break;

        // case Key::KEY_PERIOD:
        //     evt.virtual_key_code = KeyCodes::GK_PERIOD;
        //     break;

        // case Key::KEY_SLASH:
        //     evt.virtual_key_code = KeyCodes::GK_SLASH;
        //     break;

    case Key::KEY_0:
        evt.virtual_key_code = KeyCodes::GK_0;
        break;

    case Key::KEY_1:
        evt.virtual_key_code = KeyCodes::GK_1;
        break;

    case Key::KEY_2:
        evt.virtual_key_code = KeyCodes::GK_2;
        break;

    case Key::KEY_3:
        evt.virtual_key_code = KeyCodes::GK_3;
        break;

    case Key::KEY_4:
        evt.virtual_key_code = KeyCodes::GK_4;
        break;

    case Key::KEY_5:
        evt.virtual_key_code = KeyCodes::GK_5;
        break;

    case Key::KEY_6:
        evt.virtual_key_code = KeyCodes::GK_6;
        break;

    case Key::KEY_7:
        evt.virtual_key_code = KeyCodes::GK_7;
        break;

    case Key::KEY_8:
        evt.virtual_key_code = KeyCodes::GK_8;
        break;

    case Key::KEY_9:
        evt.virtual_key_code = KeyCodes::GK_9;
        break;

        // case Key::KEY_COLON:
        //     evt.virtual_key_code = KeyCodes::GK_COLON;
        //     break;

        // case Key::KEY_SEMICOLON:
        //     evt.virtual_key_code = KeyCodes::GK_SEMICOLON;
        //     break;

        // case Key::KEY_LESS:
        //     evt.virtual_key_code = KeyCodes::GK_LESS;
        //     break;

        // case Key::KEY_EQUAL:
        //     evt.virtual_key_code = KeyCodes::GK_EQUAL;
        //     break;

        // case Key::KEY_GREATER:
        //     evt.virtual_key_code = KeyCodes::GK_GREATER;
        //     break;

        // case Key::KEY_QUESTION:
        //     evt.virtual_key_code = KeyCodes::GK_QUESTION;
        //     break;

        // case Key::KEY_AT:
        //     evt.virtual_key_code = KeyCodes::GK_AT;
        //     break;

    case Key::KEY_A:
        evt.virtual_key_code = KeyCodes::GK_A;
        break;

    case Key::KEY_B:
        evt.virtual_key_code = KeyCodes::GK_B;
        break;

    case Key::KEY_C:
        evt.virtual_key_code = KeyCodes::GK_C;
        break;

    case Key::KEY_D:
        evt.virtual_key_code = KeyCodes::GK_D;
        break;

    case Key::KEY_E:
        evt.virtual_key_code = KeyCodes::GK_E;
        break;

    case Key::KEY_F:
        evt.virtual_key_code = KeyCodes::GK_F;
        break;

    case Key::KEY_G:
        evt.virtual_key_code = KeyCodes::GK_G;
        break;

    case Key::KEY_H:
        evt.virtual_key_code = KeyCodes::GK_H;
        break;

    case Key::KEY_I:
        evt.virtual_key_code = KeyCodes::GK_I;
        break;

    case Key::KEY_J:
        evt.virtual_key_code = KeyCodes::GK_J;
        break;

    case Key::KEY_K:
        evt.virtual_key_code = KeyCodes::GK_K;
        break;

    case Key::KEY_L:
        evt.virtual_key_code = KeyCodes::GK_L;
        break;

    case Key::KEY_M:
        evt.virtual_key_code = KeyCodes::GK_M;
        break;

    case Key::KEY_N:
        evt.virtual_key_code = KeyCodes::GK_N;
        break;

    case Key::KEY_O:
        evt.virtual_key_code = KeyCodes::GK_O;
        break;

    case Key::KEY_P:
        evt.virtual_key_code = KeyCodes::GK_P;
        break;

    case Key::KEY_Q:
        evt.virtual_key_code = KeyCodes::GK_Q;
        break;

    case Key::KEY_R:
        evt.virtual_key_code = KeyCodes::GK_R;
        break;

    case Key::KEY_S:
        evt.virtual_key_code = KeyCodes::GK_S;
        break;

    case Key::KEY_T:
        evt.virtual_key_code = KeyCodes::GK_T;
        break;

    case Key::KEY_U:
        evt.virtual_key_code = KeyCodes::GK_U;
        break;

    case Key::KEY_V:
        evt.virtual_key_code = KeyCodes::GK_V;
        break;

    case Key::KEY_W:
        evt.virtual_key_code = KeyCodes::GK_W;
        break;

    case Key::KEY_X:
        evt.virtual_key_code = KeyCodes::GK_X;
        break;

    case Key::KEY_Y:
        evt.virtual_key_code = KeyCodes::GK_Y;
        break;

    case Key::KEY_Z:
        evt.virtual_key_code = KeyCodes::GK_Z;
        break;

        // case Key::KEY_BRACKETLEFT:
        //     evt.virtual_key_code = KeyCodes::GK_BRACKETLEFT;
        //     break;

        // case Key::KEY_BACKSLASH:
        //     evt.virtual_key_code = KeyCodes::GK_BACKSLASH;
        //     break;

        // case Key::KEY_BRACKETRIGHT:
        //     evt.virtual_key_code = KeyCodes::GK_BRACKETRIGHT;
        //     break;

        // case Key::KEY_ASCIICIRCUM:
        //     evt.virtual_key_code = KeyCodes::GK_ASCIICIRCUM;
        //     break;

        // case Key::KEY_UNDERSCORE:
        //     evt.virtual_key_code = KeyCodes::GK_UNDERSCORE;
        //     break;

        // case Key::KEY_QUOTELEFT:
        //     evt.virtual_key_code = KeyCodes::GK_QUOTELEFT;
        //     break;

        // case Key::KEY_BRACELEFT:
        //     evt.virtual_key_code = KeyCodes::GK_BRACELEFT;
        //     break;

        // case Key::KEY_BAR:
        //     evt.virtual_key_code = KeyCodes::GK_BAR;
        //     break;

        // case Key::KEY_BRACERIGHT:
        //     evt.virtual_key_code = KeyCodes::GK_BRACERIGHT;
        //     break;

        // case Key::KEY_ASCIITILDE:
        //     evt.virtual_key_code = KeyCodes::GK_ASCIITILDE;
        //     break;

        // case Key::KEY_YEN:
        //     evt.virtual_key_code = KeyCodes::GK_YEN;
        //     break;

        // case Key::KEY_SECTION:
        //     evt.virtual_key_code = KeyCodes::GK_SECTION;
        //     break;

    default:
        UtilityFunctions::print("pressing unknwon key: ", keycode);
        evt.virtual_key_code = KeyCodes::GK_UNKNOWN;
    }
    return evt;
}

} // namespace events
} // namespace gdbind

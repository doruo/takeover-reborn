// Keyboard.h - flash.ui.Keyboard key codes, and their SDL equivalents.
#pragma once

#include <SDL3/SDL_keycode.h>

namespace engine::Keyboard {

    // The codes the Flash Player reports in KeyboardEvent.keyCode (Windows
    // virtual key codes). Letters and digits use their uppercase ASCII value.
    inline constexpr unsigned BACKSPACE = 8;
    inline constexpr unsigned TAB = 9;
    inline constexpr unsigned ENTER = 13;
    inline constexpr unsigned SHIFT = 16;
    inline constexpr unsigned CONTROL = 17;
    inline constexpr unsigned ALTERNATE = 18;
    inline constexpr unsigned CAPS_LOCK = 20;
    inline constexpr unsigned ESCAPE = 27;
    inline constexpr unsigned SPACE = 32;
    inline constexpr unsigned PAGE_UP = 33;
    inline constexpr unsigned PAGE_DOWN = 34;
    inline constexpr unsigned END = 35;
    inline constexpr unsigned HOME = 36;
    inline constexpr unsigned LEFT = 37;
    inline constexpr unsigned UP = 38;
    inline constexpr unsigned RIGHT = 39;
    inline constexpr unsigned DOWN = 40;
    inline constexpr unsigned INSERT = 45;
    inline constexpr unsigned DELETE_KEY = 46; // DELETE is a macro on Windows
    inline constexpr unsigned NUMBER_0 = 48;
    inline constexpr unsigned A = 65;
    inline constexpr unsigned NUMPAD_0 = 96;
    inline constexpr unsigned F1 = 112;

    /// Translates an SDL key to its Flash code (0 if Flash has none).
    inline unsigned fromSdl(SDL_Keycode key) {
        if (key >= SDLK_A && key <= SDLK_Z)
            return A + static_cast<unsigned>(key - SDLK_A);

        if (key >= SDLK_0 && key <= SDLK_9)
            return NUMBER_0 + static_cast<unsigned>(key - SDLK_0);

        if (key >= SDLK_F1 && key <= SDLK_F12)
            return F1 + static_cast<unsigned>(key - SDLK_F1);

        if (key >= SDLK_KP_1 && key <= SDLK_KP_9)
            return NUMPAD_0 + 1 + static_cast<unsigned>(key - SDLK_KP_1);

        switch (key) {
        case SDLK_KP_0: return NUMPAD_0;
        case SDLK_BACKSPACE: return BACKSPACE;
        case SDLK_TAB: return TAB;
        case SDLK_RETURN:
        case SDLK_KP_ENTER: return ENTER;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT: return SHIFT;
        case SDLK_LCTRL:
        case SDLK_RCTRL: return CONTROL;
        case SDLK_LALT:
        case SDLK_RALT: return ALTERNATE;
        case SDLK_CAPSLOCK: return CAPS_LOCK;
        case SDLK_ESCAPE: return ESCAPE;
        case SDLK_SPACE: return SPACE;
        case SDLK_PAGEUP: return PAGE_UP;
        case SDLK_PAGEDOWN: return PAGE_DOWN;
        case SDLK_END: return END;
        case SDLK_HOME: return HOME;
        case SDLK_LEFT: return LEFT;
        case SDLK_UP: return UP;
        case SDLK_RIGHT: return RIGHT;
        case SDLK_DOWN: return DOWN;
        case SDLK_INSERT: return INSERT;
        case SDLK_DELETE: return DELETE_KEY;
        case SDLK_KP_MULTIPLY: return 106;
        case SDLK_KP_PLUS: return 107;
        case SDLK_KP_MINUS: return 109;
        case SDLK_KP_PERIOD: return 110;
        case SDLK_KP_DIVIDE: return 111;
        case SDLK_SEMICOLON: return 186;
        case SDLK_EQUALS: return 187;
        case SDLK_COMMA: return 188;
        case SDLK_MINUS: return 189;
        case SDLK_PERIOD: return 190;
        case SDLK_SLASH: return 191;
        case SDLK_GRAVE: return 192;
        case SDLK_LEFTBRACKET: return 219;
        case SDLK_BACKSLASH: return 220;
        case SDLK_RIGHTBRACKET: return 221;
        case SDLK_APOSTROPHE: return 222;
        default: return 0;
        }
    }

} // namespace engine::Keyboard

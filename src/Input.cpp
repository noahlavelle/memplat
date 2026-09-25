#include "Input.hpp"

#include <linux/input-event-codes.h>
#include <wayland-client-protocol.h>

void Input::onKey(uint32_t keycode, uint32_t state) {
    uint8_t bit;
    switch (keycode) {
    case KEY_A:
        bit = INPUT_LEFT;
        break;
    case KEY_D:
        bit = INPUT_RIGHT;
        break;
    case KEY_SPACE:
        bit = INPUT_JUMP;
        break;
    case KEY_LEFTSHIFT:
        bit = INPUT_SPRINT;
        break;
    case KEY_ENTER:
        bit = INPUT_START;
        break;
    case KEY_TAB:
        bit = INPUT_SELECT;
        break;
    default:
        return;
    }

    if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        pressed |= bit;
        held |= bit;
    } else if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
        held &= ~bit;
    }
}

void Input::onFocusLost() {
    held = 0;
    pressed = 0;
}

void Input::beginNextFrame() { pressed = 0; }

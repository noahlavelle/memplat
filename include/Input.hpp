#pragma once

#include <cstdint>

enum InputBit : uint8_t {
    INPUT_LEFT = 1 << 0,
    INPUT_RIGHT = 1 << 1,
    INPUT_JUMP = 1 << 2,
    INPUT_SPRINT = 1 << 3,
    INPUT_START = 1 << 4,
    INPUT_SELECT = 1 << 5,
};

class Input {
  public:
    // updates the pressed bit mask
    void onKey(uint32_t keycode, uint32_t state);
    // reset both bit masks
    void onFocusLost();
    // prepares the held and pressed bit mask for the upcoming frame
    void beginFrame();

    uint8_t held = 0;
    uint8_t pressed = 0;
};

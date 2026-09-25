#pragma once

#include <cstdint>

// implemented by whatever fills the framebuffer for the frame about to be presented; pixels is
// width * height RGB565 values in row-major order. Platform owns the buffer format and
// presentation only - tiles, scrolling, sprites, a status bar, whatever - all of that is the
// implementer's problem
class FrameSource {
  public:
    virtual ~FrameSource() = default;
    virtual void render(uint16_t *pixels, int width, int height) = 0;
};

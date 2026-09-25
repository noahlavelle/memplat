#pragma once

#include "FrameSource.hpp"

class Viewport;
class PaletteMemory;

// draws the tile grid into a raw framebuffer from the platform provider
class Renderer : public FrameSource {
  public:
    Renderer(Viewport *level_viewport, PaletteMemory *palettes)
        : level_viewport(level_viewport), palettes(palettes) {}

    // fills RGB565 pixels for the frame about to be shown
    void render(uint16_t *pixels, int width, int height) override;

  private:
    Viewport *level_viewport;
    PaletteMemory *palettes;
};

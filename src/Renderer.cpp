#include "Renderer.hpp"
#include <stdexcept>

#define RGFW_IMPLEMENTATION
#include "RGFW.h"

RGFW_window *initWindow() {
    RGFW_init("memplat", 0);

    RGFW_window *win = RGFW_createWindow("memplat", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                                         RGFW_windowCenter | RGFW_windowNoResize);
    if (!win) {
        throw std::runtime_error("failed to create window");
    }
    RGFW_window_setExitKey(win, RGFW_keyEscape);

    return win;
}

RGFW_surface *createFrameSurface(RGFW_window *win, std::vector<uint8_t> &pixels) {
    RGFW_surface *surface =
        RGFW_window_createSurface(win, pixels.data(), WINDOW_WIDTH, WINDOW_HEIGHT, RGFW_formatRGB8);
    if (!surface) {
        throw std::runtime_error("failed to create surface");
    }
    return surface;
}

static void fillTileBlock(std::vector<uint8_t> &pixels, int col, int row, int offset, int size,
                          uint8_t r, uint8_t g, uint8_t b) {
    for (int y = 0; y < size; ++y) {
        int py = row * size + y;
        for (int x = 0; x < size; ++x) {
            int px = col * size + x - offset;
            if (px < 0 || px >= WINDOW_WIDTH) {
                continue;
            }
            size_t i = (static_cast<size_t>(py) * WINDOW_WIDTH + px) * 3;
            pixels[i + 0] = r;
            pixels[i + 1] = g;
            pixels[i + 2] = b;
        }
    }
}

void renderFrame(const Viewport &viewport, RGFW_window *win, RGFW_surface *surface,
                 std::vector<uint8_t> &pixels) {
    for (int row = 0; row < WINDOW_HEIGHT / 16; ++row) {
        // load one col extra so we can partially display it; handling for fine offsets
        for (int col = 0; col <= WINDOW_WIDTH / 16; ++col) {
            TileRef tile = viewport.tileAt(col, row);

            uint8_t r, g, b;
            if (tile == 0) {
                r = 135;
                g = 207;
                b = 235; // light blue: no tile
            } else {
                float shade = static_cast<float>(tile) / 15.0f;
                r = static_cast<uint8_t>((shade / 2.0f) * 255.0f);
                g = static_cast<uint8_t>(shade * 255.0f);
                b = 0;
            }

            fillTileBlock(pixels, col, row, viewport.fineOffset(), 16, r, g, b);
        }
    }

    RGFW_window_blitSurface(win, surface);
}

void freeSurface(RGFW_surface *surface) { RGFW_surface_free(surface); }

void pollEvents() { RGFW_pollEvents(); }

bool windowShouldClose(RGFW_window *win) { return RGFW_window_shouldClose(win) != RGFW_FALSE; }

void closeWindow(RGFW_window *win) { RGFW_window_close(win); }

void shutdownRenderer() { RGFW_deinit(); }

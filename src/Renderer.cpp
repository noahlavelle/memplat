#include "Renderer.hpp"
#include "LevelParser.hpp"
#include "Palette.hpp"
#include "Viewport.hpp"

#include <algorithm>
#include <array>
#include <cstring>

namespace {
constexpr int TILE_SIZE = 16;
}

void Renderer::render(uint16_t *pixels, int width, int height) {
    int scroll = level_viewport->fineOffset();

    for (int row = 0; row < height / TILE_SIZE; ++row) {
        // load one col extra to partially display it; fine offset handling
        for (int col = 0; col <= width / TILE_SIZE; ++col) {
            TileRef tile = level_viewport->tileAt(col, row);

            // TODO: get the target palette from a sprite / tile header; for now use BG OVERWORLD
            // GROUND
            Palette palette = palettes->getPalette(4);
            int colour_index = 0;
            if (tile) {
                colour_index = (tile % 3) + 1;
            }
            uint16_t pixel = palette.color_rgb[colour_index];
            std::array<uint16_t, TILE_SIZE> tile_row;
            // currently tiles are solid colour blocks so this is acceptable
            tile_row.fill(pixel);

            int pixel_x = col * TILE_SIZE - scroll;
            int pixel_y = row * TILE_SIZE;

            int clipped_x = std::max(pixel_x, 0);
            int visible_width = std::min(pixel_x + TILE_SIZE, width) - clipped_x;
            if (visible_width <= 0) {
                continue;
            }
            int src_x = clipped_x - pixel_x;

            for (int ty = 0; ty < TILE_SIZE; ++ty) {
                uint16_t *dest = pixels + (pixel_y + ty) * width + clipped_x;
                std::memcpy(dest, tile_row.data() + src_x,
                            static_cast<std::size_t>(visible_width) * sizeof(uint16_t));
            }
        }
    }
}

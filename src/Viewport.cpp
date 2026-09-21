#include "Viewport.hpp"
#include "LevelParser.hpp"

Viewport::Viewport(LevelParser &parser) : parser(parser) {};

void Viewport::advance(int d_pixel_x) {
    if (at_end) {
        return;
    }

    int next_pixel_x = pixel_x + d_pixel_x;
    int course_x = pixel_x / 16;
    int displayed_screen = course_x / SCREEN_TILE_WIDTH;

    if ((next_pixel_x / 16) / SCREEN_TILE_WIDTH != displayed_screen) {
        if (!parser.loadNextScreen()) {
            at_end = true;
        }
    }

    pixel_x = next_pixel_x;
}

TileRef Viewport::tileAt(int col, int row) const {
    int course_x = pixel_x / 16;
    int screen_x = course_x % SCREEN_TILE_WIDTH;
    int displayed_screen = course_x / SCREEN_TILE_WIDTH;

    int offset_x = screen_x + col;
    int buffer_slot = (displayed_screen + (offset_x / SCREEN_TILE_WIDTH)) & 1;
    int tile_col = offset_x % SCREEN_TILE_WIDTH;

    return parser.tileAt(buffer_slot, tile_col, row);
}

int Viewport::fineOffset() const { return pixel_x % 16; }

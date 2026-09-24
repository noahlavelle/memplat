#include "PaletteMapping.hpp"
#include "Fatal.hpp"

std::vector<std::pair<uint8_t, uint8_t>> getPaletteMapping(PaletteArchetype archetype) {
    switch (archetype) {
    case PaletteArchetype::OVERWORLD:
        // mario, koopa green, koopa red, goomba, overworld ground, hills & bushes,
        // question block, hud
        return {{0, 0}, {4, 1}, {8, 2}, {9, 3}, {12, 4}, {14, 5}, {16, 6}, {18, 7}};
    case PaletteArchetype::UNDERGROUND:
        // mario, koopa green, koopa red, buzzy beetle, underground, clouds, pipe, hud
        return {{0, 0}, {4, 1}, {8, 2}, {11, 3}, {13, 4}, {15, 5}, {17, 6}, {18, 7}};
    case PaletteArchetype::CASTLE:
        // mario, hammer bro, koopa red, buzzy beetle, castle bricks, clouds, pipe, hud
        return {{0, 0}, {6, 1}, {8, 2}, {11, 3}, {20, 4}, {15, 5}, {17, 6}, {18, 7}};
    case PaletteArchetype::NIGHT_OVERWORLD:
        // same cast as overworld, night ground swapped in for the sky colour
        return {{0, 0}, {4, 1}, {8, 2}, {9, 3}, {21, 4}, {14, 5}, {16, 6}, {18, 7}};
    case PaletteArchetype::SPECIAL_STAGE:
        // fire mario, lakitu, koopa red, bullet bill, overworld ground, hills & bushes,
        // question block, star & special
        return {{2, 0}, {7, 1}, {8, 2}, {10, 3}, {12, 4}, {14, 5}, {16, 6}, {19, 7}};
    default:
        fatal("unknown palette archetype");
    }
}

#pragma once

#include <cstdint>
#include <utility>
#include <vector>

// 3-bit, one variant per level 'theme'; can be extended with alternate hardmode variants
enum PaletteArchetype : uint8_t {
    OVERWORLD = 0,
    UNDERGROUND = 1,
    CASTLE = 2,
    NIGHT_OVERWORLD = 3,
    SPECIAL_STAGE = 4,
};

// palette id -> slot id, for the given level archetype
std::vector<std::pair<uint8_t, uint8_t>> getPaletteMapping(PaletteArchetype archetype);

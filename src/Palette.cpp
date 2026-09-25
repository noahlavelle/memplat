#include "Palette.hpp"
#include "ByteReader.hpp"
#include "Fatal.hpp"
#include <cstddef>

namespace {
// slots: player, enemy A, enemy B, enemy C, terrain, scenery, interactive, ui
constexpr PaletteMapping MAPPINGS[] = {
    /* OVERWORLD */ {0, 4, 8, 9, 12, 14, 16, 18},
    /* UNDERGROUND */ {0, 4, 8, 11, 13, 15, 17, 18},
    /* CASTLE */ {0, 6, 8, 11, 20, 15, 17, 18},
    /* NIGHT_OVERWORLD */ {0, 4, 8, 9, 21, 14, 16, 18},
    /* SPECIAL_STAGE */ {2, 7, 8, 10, 12, 14, 16, 19},
};
} // namespace

const PaletteMapping &getPaletteMapping(PaletteArchetype archetype) {
    if (archetype >= std::size(MAPPINGS)) {
        fatal("unknown palette archetype");
    }
    return MAPPINGS[archetype];
}

PaletteMemory::PaletteMemory(ByteReader reader) : reader(std::move(reader)) {}

Palette PaletteMemory::getPalette(unsigned int slot) const {
    size_t beginning = slot_to_id[slot] * PALETTE_BYTES;

    Palette p;
    for (int i = 0; i < 4; ++i) {
        p.color_rgb[i] = (std::to_integer<uint16_t>(*reader.at(beginning + i * 2)) << 8) |
                         std::to_integer<uint16_t>(*reader.at(beginning + i * 2 + 1));
    }
    return p;
}

bool PaletteMemory::checkId(uint8_t palette_id) const {
    return reader.inBounds(static_cast<size_t>(palette_id) * PALETTE_BYTES + PALETTE_BYTES - 1);
}

void PaletteMemory::loadPalettes(const PaletteMapping &slot_mapping) {
    for (uint8_t palette_id : slot_mapping) {
        if (!checkId(palette_id)) {
            fatal("palette id out of bounds");
        }
    }
    slot_to_id = slot_mapping;
}

void PaletteMemory::replacePalette(uint8_t palette_id, uint8_t slot) {
    if (slot > 7) {
        fatal("slot out of bounds");
    }
    if (!checkId(palette_id)) {
        fatal("palette id out of bounds");
    }
    slot_to_id[slot] = palette_id;
}

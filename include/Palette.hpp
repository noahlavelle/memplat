#pragma once

#include "ByteReader.hpp"
#include <array>
#include <cstdint>

struct Palette {
    uint16_t color_rgb[4];
};

// 3-bit, one variant per level 'theme'; can be extended with alternate hardmode variants
enum PaletteArchetype : uint8_t {
    OVERWORLD = 0,
    UNDERGROUND = 1,
    CASTLE = 2,
    NIGHT_OVERWORLD = 3,
    SPECIAL_STAGE = 4,
};

// maps palette ids in palettes file to slot indexes
using PaletteMapping = std::array<uint8_t, 8>;
const PaletteMapping &getPaletteMapping(PaletteArchetype archetype);

class PaletteMemory {
  public:
    explicit PaletteMemory(ByteReader reader);

    void loadPalettes(const PaletteMapping &slot_mapping);
    void replacePalette(uint8_t palette_id, uint8_t slot);
    Palette getPalette(unsigned int slot) const;

  private:
    static constexpr size_t PALETTE_BYTES = 8;

    bool checkId(uint8_t palette_id) const;

    ByteReader reader;
    PaletteMapping slot_to_id = {};
};

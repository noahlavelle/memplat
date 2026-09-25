#pragma once

#include "ByteReader.hpp"
#include <cstdint>
#include <vector>

struct Palette {
    uint16_t color_rgb[4];
};

class PaletteMemory {
  public:
    explicit PaletteMemory(ByteReader reader);

    Palette getPalette(unsigned int slot) const;
    // mapping should be palette id (index) -> slot index; repeated palette ids are supported
    void loadPalettes(std::vector<std::pair<uint8_t, uint8_t>> slot_mapping);
    // in-place overwrite of a single palette; has to reload and scan the palette file so use rarely
    void replacePalette(uint8_t palette_id, uint8_t slot);

  private:
    ByteReader reader;
    Palette palettes[8];

    // consume the next 8 bytes, mapping them to a palette
    Palette readPalette();
    // discard the next 8 bytes
    void discardPalette();
};

#include "Palette.hpp"
#include "ByteReader.hpp"
#include "Fatal.hpp"
#include <algorithm>
#include <functional>

PaletteMemory::PaletteMemory(ByteReader reader) : reader(std::move(reader)) {}

Palette PaletteMemory::readPalette() {
    auto palette = Palette{0};

    for (int i = 0; i < 4; ++i) {
        auto col_high = reader.consume();
        if (!col_high) {
            fatal("malformed palette data");
        }
        auto col_low = reader.consume();
        if (!col_low) {
            fatal("malformed palette data");
        }
        uint16_t colour = (static_cast<uint16_t>(*col_high) << 8) | static_cast<uint16_t>(*col_low);
        palette.color_rgb[i] = colour;
    }

    return palette;
}

void PaletteMemory::discardPalette() {
    for (int i = 0; i < 8; ++i) {
        if (!reader.consume()) {
            fatal("malformed palette data");
        }
    }
}

Palette PaletteMemory::getPalette(unsigned int slot) const { return palettes[slot]; }

void PaletteMemory::loadPalettes(std::vector<std::pair<uint8_t, uint8_t>> slot_mapping) {
    reader.rewind();
    // sorted palette id -> slot id, so we can trust this to walk the palettes file in order
    std::sort(slot_mapping.begin(), slot_mapping.end(), std::ranges::greater());

    unsigned int palette_at = 0;
    while (!slot_mapping.empty()) {
        auto [index, slot] = slot_mapping.back();

        if (palette_at == index) {
            Palette p = readPalette();
            palettes[slot] = p;
            slot_mapping.pop_back();

            // fill any other palettes with the same index
            while (!slot_mapping.empty() && slot_mapping.back().first == index) {
                palettes[slot_mapping.back().second] = p;
                slot_mapping.pop_back();
            }
        } else {
            discardPalette();
        }

        ++palette_at;
    }
}

void PaletteMemory::replacePalette(uint8_t palette_id, uint8_t slot) {
    if (slot > 7) {
        fatal("slot out of bounds");
    }

    reader.rewind();
    for (int i = 0; i <= palette_id; ++i) {
        if (i == palette_id) {
            palettes[slot] = readPalette();
        } else {
            discardPalette();
        }
    }
}

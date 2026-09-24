#pragma once

#include "ByteReader.hpp"
#include "Palette.hpp"
#include "PaletteMapping.hpp"
#include <cstddef>
#include <cstdint>

using TileRef = uint8_t;

constexpr int SCREEN_TILE_WIDTH = 16;
constexpr int SCREEN_TILE_HEIGHT = 16;

enum TimerDuration : uint8_t {
    LOW = 0,
    MEDIUM = 1,
    HIGH = 2,
};

// holds a single screen of resolved tiles in a buffer
struct LevelBuffer {
    TileRef tiles[SCREEN_TILE_WIDTH][SCREEN_TILE_HEIGHT] = {};
};

class LevelParser {
  public:
    // aborts if the level data is malformed or truncated
    explicit LevelParser(ByteReader level_reader, PaletteMemory *palettes);

    // true if a new screen was loaded, false at a clean end of file (aborts on malformed or
    // truncated data instead of returning an error)
    bool loadNextScreen();
    TileRef tileAt(int slot, int col, int row) const;

  private:
    ByteReader reader;
    PaletteMemory *palettes;
    LevelBuffer buffers[2];
    // only ever accumulates forward through the level binary; never negative
    unsigned int acc_x = 0;

    PaletteArchetype level_archetype;
    bool level_hardmode;
    TimerDuration level_timer;
    bool level_spawn_on_ground;
    bool level_autowalk;

    void loadHeader();
    void loadObject(std::byte coordinate_byte, std::byte object_byte);
    void parseReservedRow(int x, int y, int type, int data);
    void parseSingleTile(int x, int y, int tile);
    void parseMetaTile(int x, int y, int metatile, int data);
};

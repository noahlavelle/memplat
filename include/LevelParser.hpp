// LevelParser.h
#ifndef LEVELPARSER_H
#define LEVELPARSER_H

#include "ByteReader.hpp"
#include <cstddef>
#include <cstdint>

using TileRef = uint8_t;

constexpr int SCREEN_TILE_WIDTH = 16;
constexpr int SCREEN_TILE_HEIGHT = 16;

// holds a single screen of resolved tiles in a buffer
struct LevelBuffer {
    TileRef tiles[SCREEN_TILE_WIDTH][SCREEN_TILE_HEIGHT] = {};
};

class LevelParser {
  public:
    // aborts if the level data is malformed or truncated
    static LevelParser create(ByteReader reader);

    // true if a new screen was loaded, false at a clean end of file (aborts on malformed or
    // truncated data instead of returning an error)
    bool loadNextScreen();
    TileRef tileAt(int slot, int col, int row) const;

  private:
    explicit LevelParser(ByteReader reader) : reader(std::move(reader)) {}

    ByteReader reader;
    LevelBuffer buffers[2];
    int acc_x = 0;

    void loadObject(std::byte coordinate_byte, std::byte object_byte);
    void parseReservedRow(int x, int y, int type, int data);
    void parseSingleTile(int x, int y, int tile);
    void parseMetaTile(int x, int y, int metatile, int data);
};

#endif // LEVELPARSER_H

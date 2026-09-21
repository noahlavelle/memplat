#include "LevelParser.hpp"
#include "ByteReader.hpp"
#include "Errors.hpp"
#include <cstddef>
#include <optional>

/*
Decodes according to the format currently defined in readme.
*/

// objects cannot spawn on the last 2 rows; x=15 on the y=15 reservation marks a termination
const int TERMINATOR = 0xFF;
// empty type id, following data should be treated as a single tile id
const int SINGLE_TILE_MODE = 0x0;
// rows above reserved, inclusive; top row only
const int RESERVED_TOP_CUTOFF = 0xF;
// rows below reserved, inclusive; bottom two rows, 0-1
const int RESERVED_BOTTOM_CUTOFF = 0x1;
// maximum x value able to be encoded
const int SCREEN_WIDTH = 0xF;

namespace {
// byte masks, splits byte groups into high and low parts; e.g x & y
constexpr std::byte HIGH_NIBBLE_MASK{0xF0};
constexpr std::byte LOW_NIBBLE_MASK{0x0F};
constexpr int METATILE_SIZE_MASK{0x3};
constexpr int METATILE_SIZE_SHIFT = 2;

int getHigh(std::byte b) { return std::to_integer<int>((b & HIGH_NIBBLE_MASK) >> 4); }
int getLow(std::byte b) { return std::to_integer<int>(b & LOW_NIBBLE_MASK); }
int getMetatileHigh(int d) { return (d >> METATILE_SIZE_SHIFT) & METATILE_SIZE_MASK; }
int getMetatileLow(int d) { return d & METATILE_SIZE_MASK; }
} // namespace

LevelParser::LevelParser(ByteReader reader) : reader(std::move(reader)) {}

void LevelParser::loadScreen() {
    while (true) {
        // coordinate byte: xxxxyyyy
        auto coordinate_byte = reader.consume();
        if (!coordinate_byte) {
            throw LevelParseError("unexpected end of file while reading coordinate byte",
                                   reader.position());
        }
        if (static_cast<int>(*coordinate_byte) == TERMINATOR) {
            return;
        }

        // object byte: ttttdddd
        auto object_byte = reader.consume();
        if (!object_byte) {
            throw LevelParseError("unexpected end of file while reading object byte",
                                   reader.position());
        }

        loadObject(*coordinate_byte, *object_byte);

        if (crossedScreenBoundary()) {
            return;
        }
    }
}

void LevelParser::loadObject(std::byte coordinate_byte, std::byte object_byte) {
    int x = getHigh(coordinate_byte);
    int y = getLow(coordinate_byte);

    int type = getHigh(object_byte);
    int data = getLow(object_byte);

    if (y >= RESERVED_TOP_CUTOFF || y <= RESERVED_BOTTOM_CUTOFF) {
        parseReservedRow(type, data);
    } else if (type == SINGLE_TILE_MODE) {
        parseSingleTile(data);
    } else {
        parseMetaTile(type, data);
    }

    levelXCounter += x;
}

// row has reserved meaning, override default type decoding
void LevelParser::parseReservedRow(int type, int data) {
    std::fprintf(stdout, "Reserved row object, type %d, data %d\n", type, data);
}

// not a metatile, object byte: 0000tttt
void LevelParser::parseSingleTile(int tile) {
    std::fprintf(stdout, "Single tile object, tile %d\n", tile);
}

// metatile, object byte: tttt(lx)(lx)(ly)(ly)
// metatile length are either 0, 1, or 2; depending on the tile they map to 1x, 2x and 4x
// mults, or predefined static sizes
void LevelParser::parseMetaTile(int metatile, int data) {
    int lx = getMetatileHigh(data);
    int ly = getMetatileLow(data);

    std::fprintf(stdout, "Metatile object, metatile %d, lx %d, ly %d\n", metatile, lx, ly);
}

// if the x counter is a multiple of screensize and the next object advances the position, stop
bool LevelParser::crossedScreenBoundary() {
    if (levelXCounter % SCREEN_WIDTH != 0) {
        return false;
    }
    auto next_coordinate = reader.peek();
    if (!next_coordinate) {
        throw LevelParseError("unexpected end of file while checking screen boundary",
                               reader.position());
    }

    return getHigh(*next_coordinate) != 0;
}

#include "LevelParser.hpp"
#include "ByteReader.hpp"
#include "Fatal.hpp"
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

LevelParser LevelParser::create(ByteReader reader) {
    LevelParser parser(std::move(reader));

    // populate both buffer slots on load
    if (parser.loadNextScreen()) {
        parser.loadNextScreen();
    }
    return parser;
}

bool LevelParser::loadNextScreen() {
    std::optional<unsigned int> screen_index;

    while (true) {
        // coordinate byte: xxxxyyyy (peeked, not consumed until we know it's ours)
        auto coordinate_byte = reader.peek();
        if (!coordinate_byte) {
            fatal("malformed level data: unexpected end of file, expected coordinate byte");
        }
        if (static_cast<int>(*coordinate_byte) == TERMINATOR) {
            reader.consume();
            return false;
        }

        int x = getHigh(*coordinate_byte);
        unsigned int candidate_screen = (acc_x + x) / SCREEN_TILE_WIDTH;

        if (!screen_index) {
            screen_index = candidate_screen;
        } else if (candidate_screen != *screen_index) {
            return true;
        }

        reader.consume();

        // object byte: ttttdddd
        auto object_byte = reader.consume();
        if (!object_byte) {
            fatal("malformed level data: unexpected end of file, expected object byte");
        }

        loadObject(*coordinate_byte, *object_byte);
    }
}

TileRef LevelParser::tileAt(int slot, int col, int row) const {
    return buffers[slot].tiles[col][row];
}

void LevelParser::loadObject(std::byte coordinate_byte, std::byte object_byte) {
    int x = getHigh(coordinate_byte);
    int y = getLow(coordinate_byte);

    int type = getHigh(object_byte);
    int data = getLow(object_byte);

    acc_x += x;
    int column = acc_x % SCREEN_TILE_WIDTH;

    if (y >= RESERVED_TOP_CUTOFF || y <= RESERVED_BOTTOM_CUTOFF) {
        parseReservedRow(column, y, type, data);
    } else if (type == SINGLE_TILE_MODE) {
        parseSingleTile(column, y, data);
    } else {
        parseMetaTile(column, y, type, data);
    }
}

// row has reserved meaning, override default type decoding
void LevelParser::parseReservedRow(int x, int y, int type, int data) {}

// not a metatile, object byte: 0000tttt
void LevelParser::parseSingleTile(int x, int y, int tile) {
    buffers[(acc_x / SCREEN_TILE_WIDTH) & 1].tiles[x][y] = tile;
}

// metatile, object byte: tttt(lx)(lx)(ly)(ly)
// metatile length are either 0, 1, or 2; depending on the tile they map to 1x, 2x and 4x
// mults, or predefined static sizes
void LevelParser::parseMetaTile(int x, int y, int metatile, int data) {
    int lx = getMetatileHigh(data);
    int ly = getMetatileLow(data);
}

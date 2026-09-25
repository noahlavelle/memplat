#include "LevelParser.hpp"
#include "ByteReader.hpp"
#include "Fatal.hpp"
#include "Palette.hpp"
#include <cstddef>
#include <optional>

/*
Decodes according to the format currently defined in readme.
*/

// objects cannot spawn on the last 2 rows; x=15 on the y=15 reservation marks a termination
const int TERMINATOR = 0xFF;
// empty type id, following data should be treated as a single tile id
const int SINGLE_TILE_MODE = 0x0;
// reserved if y <= this value; currently just the top row (y = 0)
const int RESERVED_TOP_CUTOFF = 0x0;
// reserved if y >= this value; currently the bottom two rows (y = 14, 15)
const int RESERVED_BOTTOM_CUTOFF = 0xE;

namespace {
// byte masks, splits byte groups into high and low parts; e.g x & y
constexpr std::byte HIGH_NIBBLE_MASK{0xF0};
constexpr std::byte LOW_NIBBLE_MASK{0x0F};
constexpr int METATILE_SIZE_MASK{0x3};
constexpr int METATILE_SIZE_SHIFT = 2;

// header is AAABCCDE; letters represent below constants in order
constexpr std::byte HEADER_ARCHETYPE_MASK{0xE0};
constexpr std::byte HEADER_HARDMODE_MASK{0x10};
constexpr std::byte HEADER_TIMER_MASK{0x0C};
constexpr std::byte HEADER_SPAWN_MASK{0x02};
constexpr std::byte HEADER_AUTOWALK_MASK{0x01};

int getHigh(std::byte b) { return std::to_integer<int>((b & HIGH_NIBBLE_MASK) >> 4); }
int getLow(std::byte b) { return std::to_integer<int>(b & LOW_NIBBLE_MASK); }
int getMetatileHigh(int d) { return (d >> METATILE_SIZE_SHIFT) & METATILE_SIZE_MASK; }
int getMetatileLow(int d) { return d & METATILE_SIZE_MASK; }

int getArchetype(std::byte b) { return std::to_integer<int>((b & HEADER_ARCHETYPE_MASK) >> 5); }
int getHardmode(std::byte b) { return std::to_integer<int>((b & HEADER_HARDMODE_MASK) >> 4); }
int getTimer(std::byte b) { return std::to_integer<int>((b & HEADER_TIMER_MASK) >> 2); }
int getSpawn(std::byte b) { return std::to_integer<int>((b & HEADER_SPAWN_MASK) >> 1); }
int getAutowalk(std::byte b) { return std::to_integer<int>(b & HEADER_AUTOWALK_MASK); }
} // namespace

LevelParser::LevelParser(ByteReader level_reader, PaletteMemory *palettes)
    : reader(std::move(level_reader)), palettes(palettes) {

    loadHeader();
    palettes->loadPalettes(getPaletteMapping(level_archetype));

    // populate both buffer slots on load
    if (loadNextScreen()) {
        loadNextScreen();
    }
}

void LevelParser::loadHeader() {
    auto header_byte = reader.consume();
    if (!header_byte) {
        fatal("malformed level data: no header");
    }

    level_archetype = static_cast<PaletteArchetype>(getArchetype(*header_byte));
    level_timer = static_cast<TimerDuration>(getTimer(*header_byte));
    level_hardmode = getHardmode(*header_byte);
    level_spawn_on_ground = getSpawn(*header_byte);
    level_autowalk = getAutowalk(*header_byte);
}

bool LevelParser::loadNextScreen() {
    std::optional<unsigned int> screen_index;

    while (true) {
        // coordinate byte: xxxxyyyy (peeked, not consumed until we know it's ours)
        auto coordinate_byte = reader.peek();
        if (!coordinate_byte) {
            fatal("malformed level data: unexpected end of file, expected coordinate "
                  "byte");
        }
        if (static_cast<int>(*coordinate_byte) == TERMINATOR) {
            reader.consume();
            return false;
        }

        int x = getHigh(*coordinate_byte);
        unsigned int candidate_screen = (acc_x + x) / SCREEN_TILE_WIDTH;

        if (!screen_index) {
            screen_index = candidate_screen;
            buffers[candidate_screen & 1] = LevelBuffer{};
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

    if (y >= RESERVED_BOTTOM_CUTOFF || y <= RESERVED_TOP_CUTOFF) {
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
// metatile length are either 0, 1, or 2; depending on the tile they map to 1x, 2x and
// 4x mults, or predefined static sizes
void LevelParser::parseMetaTile(int x, int y, int metatile, int data) {
    int lx = getMetatileHigh(data);
    int ly = getMetatileLow(data);
}

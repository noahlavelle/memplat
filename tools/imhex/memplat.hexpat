#pragma description Memplat level (.lvl) format
#pragma endian big

// Mirrors README.md "LEVEL FORMAT". Each entry is normally 2 bytes:
//   xxxxyyyy tttttttt
// The lone byte 0xFF (X=15, Y=15) terminates the level early — no type byte
// follows it.

import std.mem;

// --- Level header ---------------------------------------------------------

bitfield Header {
    archetype : 3; // BG/UI palette + metatile set, indexed into a fixed table (max 8).
    hard_mode : 1; // same archetype, but swaps one enemy palette slot for a tougher option.
    timer     : 2; // enum, values TBD.
    air_spawn : 1; // 0 = ground, 1 = air (drop-in).
    autowalk  : 1; // 0 = controllable, 1 = autowalk.
};

// --- Coordinate byte ----------------------------------------------------

bitfield Coord {
    x : 4; // Relative X: tile offset added to the running levelX counter.
           // Max jump is 15; chain null (type 0x00) objects to span further
           // than one 16-tile screen.
    y : 4; // Absolute Y (tile row).
           //  0      = mode-switch row, reused for fixed-Y objects
           //           (lakitu, flag, hole?) needing extra ref space.
           //  1..13  = normal placement.
           //  14..15 = ground pattern / other special-row encodings
           //           (holes, the terminator below).
};

// --- Type byte, two interpretations --------------------------------------

// Metatile grouping: premade multi-tile objects (pipe, staircase, hole, etc).
bitfield MetatileType {
    metatile_ref : 4; // 1-15 (0 is reserved, see SingleTileType below).
                       // Looked up per world/level's own metatile palette.
    width_mult   : 2; // 0 = 1x .. 3 = 4x
    height_mult  : 2; // 0 = 1x .. 3 = 4x
};

// Single tile: one-off placements (coin block, spring, etc).
bitfield SingleTileType {
    padding_zero : 4 [[hidden]]; // always 0000, signals single-tile mode
    tile_ref     : 4;            // 0-15, index into the tilemap
};

// --- One level object entry ----------------------------------------------

// Terminator: X=15,Y=15 packed into the coord byte alone, no type byte.
struct Terminator {
    Coord coord;
};

struct NormalObject {
    Coord coord;

    u8 type_peek = std::mem::read_unsigned($, 1, std::mem::Endian::Big) [[hidden]];

    if ((type_peek & 0xF0) == 0) {
        SingleTileType type;
    } else {
        MetatileType type;
    }

    // TODO: some metatile refs (e.g. a pipe) carry extra trailing bits/bytes
    // (a target level id, etc), consumed case-by-case once the metatile id
    // -> behaviour table exists. Not decoded here yet.
};

// Picks the right shape for the current entry based on its first byte,
// so the terminator renders as its own type instead of a half-empty object.
struct Entry {
    u8 peek = std::mem::read_unsigned($, 1, std::mem::Endian::Big) [[hidden]];

    if (peek == 0xFF) {
        Terminator terminator;
        break;
    } else {
        NormalObject object;
    }
};

// --- Whole level: a run of entries up to the terminator (or EOF) --------

struct Level {
    Header header;
    Entry entries[while(!std::mem::eof())];
};

Level level @ 0x00;

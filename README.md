# MEMPLAT

Super memory and storage optimised platformer, probably a mario clone with
constraints similar to the NES.

## LEVEL FORMAT

### Header

Every level starts with a single header byte:

```
AAABCCDE
```

- `AAA` — Archetype (3 bits): picks the level's BG/UI palette set and
  metatile palette, indexed into a fixed table (max 8 archetypes).
- `B` — Hard mode (1 bit): same archetype, but swaps one enemy palette
  slot for a tougher option (e.g. Bullet Bill in for Goomba).
- `CC` — Timer (2 bits): enum, values TBD.
- `D` — Spawn position (1 bit): `0` ground, `1` air (drop-in).
- `E` — Movement (1 bit): `0` controllable, `1` autowalk.

### Object entries

After the header, each level is a flat list of 2-byte object entries:

```
xxxxyyyytttttttt
```

- `xxxx` — Relative X (4 bits): tile offset from the previous object's X.
- `yyyy` — Absolute Y (4 bits): tile row.
- `tttttttt` — Object Type (8 bits): interpreted per the formats below.

### Coordinate space

Screen size is 16x16 tiles (both X and Y are 4-bit fields).

**X** is relative to the last object. A running `levelX` counter accumulates
each relative X. When it passes a multiple of the screen size, the renderer
stops and only continues when the next screen is needed (and the last one has
been destroyed).

Relative X can only jump 15 tiles at a time. To span more than one screen,
chain null objects to cover the remaining distance, e.g.:

```
1111000000000000
```

X=15, type=0x00 ("null", spawns nothing) — repeat as needed for longer gaps.

**Y** rows are reserved as follows:

- `Y=0` — top row; nothing spawns here, so it's reused as a decoding
  mode switch for objects that always spawn on a fixed Y (lakitu, flag, hole?
  etc.), giving extra ref space beyond what a single byte's nibbles allow.
- `Y=1..13` — normal object placement.
- `Y=14..15` — reserved for the ground pattern, and otherwise free for other
  special-row encodings (holes, the level terminator below, etc.).

**Level terminator**:

```
11111111
```

X=15, Y=15 — a reserved coordinate pair (falls within the reserved bottom
rows above). This is a single terminating byte, not a full 2-byte entry.

### Object Type (tttttttt)

The type byte is set in one of the following formats:

#### Metatile grouping

Premade multi-tile objects (pipe, staircase, hole, etc.):

```
mmmmllll        (full entry: xxxxyyyymmmmllll)
```

- `mmmm` — Metatile ref. `0000` is reserved (see Single Tile below), so only
  1-15 are usable as real metatile refs.
- `llll` — Object size multiplier, split into two 2-bit fields:
  `lxlx` (width multiplier) and `lyly` (height multiplier)
  — full entry: `xxxxyyyymmmm(lx)(lx)(ly)(ly)`.
  `0` means original size; other values map to either dynamically scaled or
  pre-encoded width/height multipliers.
  E.g. `11110101` refers to metatile 15 at 2x width and 2x height.

Metatile refs are looked up per world/level type — each has its own metatile
palette. E.g. the "hole" ref might draw as an empty void in an overworld
level but as a lava pit in a castle level.

Each metatile has a file? First bytes encode a unique id. A table maps
metatile ids to implemented behaviour (breakable blocks, pipe logic, etc.).

Special cases can add extra bits/bytes where a metatile needs more data than
fits in the entry, consumed on a case-by-case basis: when the parser reads a
metatile type that needs it (e.g. a pipe, carrying an extra 4 bits encoding a
target level id), it consumes the extra bits immediately after and tacks
them onto the loaded object data before continuing.

#### Single tile

One-off placements (coin block, spring, etc.):

```
0000ssss        (full entry: xxxxyyyy0000ssss)
```

- top nibble `0000` signals single-tile mode.
- `ssss` — Single Tile Ref (0-15).

Tiles are stored in a tilemap file; `ssss` refers to id within the map?

## SPRITES

For now consider sprites as one tile only. We have a spritemap file made up
of an 8x8 grid, defining every sprite. The sprite id is a hex ref to its
position in this grid. Like metatiles and tiles, we have a behaviour table
to map a sprite id in a given slot to implemented behaviour.

## PALETTES

The pixel buffer uses the RGB565 format, so palettes must encode colours matching
this. Each pixel in a sprite or tile represents colour with 2 bits, so palettes
can only fill 4 different options (00 for BG, 01 & 10 & 11 for actual colours).
The game can hold 8 palettes at once, 4 for background and 4 for sprites. The
exact combo of palettes loaded is determined in header data at the start of the
level. The binary data for each sprite and tile begins with a three bit number,
determining which palette slot it pulls its colours from. The slots are organised
as such:

0 - Player Character
1 - Enemy Group A
2 - Enemy Group B
3 - Enemy Group C
4 - BG Main Terrain
5 - BG Scenery
6 - BG Interactive Tiles
7 - UI & Special

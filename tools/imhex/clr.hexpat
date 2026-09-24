#pragma description Memplat palette (.clr) format
#pragma endian big

// Mirrors PALETTES.md. Flat sequence of fixed 64-bit records, no headers or
// separators between them — record N (referenced by a level header as
// "palette N") lives at byte offset N * 8. No terminator either; the file
// just ends after the last palette.

import std.mem;

struct Palette {
    u16 colors[4]; // index 0 = BG (pixel value 00), 1-3 = drawable (01/10/11)
};

Palette palettes[while(!std::mem::eof())] @ 0x00;

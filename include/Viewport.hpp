#pragma once

#include "LevelParser.hpp"

class Viewport {
  public:
    explicit Viewport(LevelParser &parser);

    void advance(unsigned int dx);
    TileRef tileAt(int col, int row) const;
    int fineOffset() const;

  private:
    LevelParser &parser;
    // only ever moves forward through the level binary; never negative
    unsigned int pixel_x = 0;
    bool at_end = false;
};

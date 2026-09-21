#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "LevelParser.hpp"

class Viewport {
  public:
    explicit Viewport(LevelParser &parser);

    void advance(int dx);
    TileRef tileAt(int col, int row) const;
    int fineOffset() const;

  private:
    LevelParser &parser;
    int pixel_x = 0;
    bool at_end = false;
};

#endif // VIEWPORT_H

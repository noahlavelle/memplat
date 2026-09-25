#pragma once

#include "Ticker.hpp"

class Viewport;
class Input;

// handles the per-tick game logic
class Game : public Ticker {
  public:
    Game(Viewport *level_viewport, Input *input) : level_viewport(level_viewport), input(input) {}

    void tick() override;

  private:
    Viewport *level_viewport;
    Input *input;
};

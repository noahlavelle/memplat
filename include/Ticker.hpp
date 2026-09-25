#pragma once

// implemented by whatever should run once per fixed-rate simulation tick
class Ticker {
  public:
    virtual ~Ticker() = default;
    virtual void tick() = 0;
};

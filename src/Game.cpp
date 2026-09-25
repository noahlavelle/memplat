#include "Game.hpp"
#include "Input.hpp"
#include "Viewport.hpp"

void Game::tick() {
    if (input->held & INPUT_RIGHT) {
        level_viewport->advance(1);
    }
}

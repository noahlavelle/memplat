#include <cstdio>
#include <optional>

#include "ByteReader.hpp"
#include "Input.hpp"
#include "LevelParser.hpp"
#include "Renderer.hpp"
#include "Viewport.hpp"

/*
TODO:
Create concept of metatile
Start resolving metatiles to tiles
Write these resolved tiles onto the grid (will have to consider the bounding box of the metatile?)
Come up with a format for tiles and metatiles
Add the renderer, to stream from the screen and draw whatever tile is at each position
Check if the colour mode is available, falling back to 32 bit colour if not
*/

const int WINDOW_WIDTH = BUFFER_WIDTH * 3;
const int WINDOW_HEIGHT = BUFFER_HEIGHT * 3;

int main(int argc, char **argv) {
    std::optional<ByteReader> reader = ByteReader::open("./data/levels/1-1.lvl");
    if (!reader) {
        fprintf(stderr, "failed to load level\n");
        return 1;
    }

    LevelParser parser = LevelParser::create(std::move(*reader));

    Viewport viewport(parser);
    Input input;
    Renderer renderer("memplat", "memplat", WINDOW_WIDTH, WINDOW_HEIGHT, &viewport, &input);
    if (!renderer.ok()) {
        fprintf(stderr, "startup failed: renderer initialization failed\n");
        return 1;
    }

    while (true) {
        input.beginFrame();

        if (!renderer.dispatch()) {
            break;
        }

        if (input.held & INPUT_RIGHT) {
            viewport.advance(1);
        }
    }

    return 0;
}

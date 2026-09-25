#include <cstdio>

#include "ByteReader.hpp"
#include "Game.hpp"
#include "Input.hpp"
#include "LevelParser.hpp"
#include "Palette.hpp"
#include "Platform.hpp"
#include "Renderer.hpp"
#include "Viewport.hpp"

const int WINDOW_WIDTH = BUFFER_WIDTH * 3;
const int WINDOW_HEIGHT = BUFFER_HEIGHT * 3;

// replace pretty printed throws on libstdc++ defaults like std::bad_alloc with aborts as to not
// pull in weighty exception mechanics
void *operator new(std::size_t size) {
    void *p = std::malloc(size ? size : 1);
    if (!p) {
        std::abort();
    }
    return p;
}

int main(int argc, char **argv) {
    auto palette_reader = ByteReader::open("./data/palettes.clr");
    if (!palette_reader) {
        fprintf(stderr, "failed to load palette\n");
        return 1;
    }

    auto level_reader = ByteReader::open("./data/levels/1-1.lvl");
    if (!level_reader) {
        fprintf(stderr, "failed to load level\n");
        return 1;
    }

    PaletteMemory palettes(std::move(*palette_reader));
    LevelParser parser(std::move(*level_reader), &palettes);

    Viewport viewport(parser);
    Input input;
    Game game(&viewport, &input);
    Renderer renderer(&viewport, &palettes);
    Platform platform("memplat", "memplat", WINDOW_WIDTH, WINDOW_HEIGHT, &input, &game, &renderer);

    while (platform.dispatch()) {
    }

    return 0;
}

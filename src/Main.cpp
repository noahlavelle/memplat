#include <cstdint>
#include <cstdio>
#include <exception>
#include <fcntl.h>
#include <optional>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "ByteReader.hpp"
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
*/

const int REFRESH_RATE = 60;

int main(int argc, char **argv) {
    RGFW_window *win = nullptr;
    RGFW_surface *surface = nullptr;
    std::vector<uint8_t> pixels;
    std::optional<LevelParser> parser;
    std::optional<Viewport> viewport;

    try {
        win = initWindow();
        pixels.resize(static_cast<size_t>(WINDOW_WIDTH) * WINDOW_HEIGHT * 3);
        surface = createFrameSurface(win, pixels);

        auto reader = ByteReader::open("./data/levels/1-1.lvl");
        parser.emplace(std::move(reader));
        viewport.emplace(*parser);
    } catch (const std::exception &e) {
        fprintf(stderr, "startup failed: %s\n", e.what());
        return 1;
    }

    while (!windowShouldClose(win)) {
        pollEvents();
        viewport->advance(1);
        renderFrame(*viewport, win, surface, pixels);

        usleep(1000000 / REFRESH_RATE);
    }

    freeSurface(surface);
    closeWindow(win);
    shutdownRenderer();

    return 0;
}

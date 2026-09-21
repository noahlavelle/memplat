#include <cstdio>
#include <exception>
#include <fcntl.h>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

#include "ByteReader.hpp"
#include "LevelParser.hpp"

#define RGFW_IMPLEMENTATION
#define RGFW_EGL
#include "RGFW.h"

#ifdef RGFW_MACOS
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

RGFW_window *initWindow() {
    RGFW_init("memplat", RGFW_initEGL);

    RGFW_glHints *hints = RGFW_getGlobalHints_OpenGL();
    hints->major = 1;
    hints->minor = 1;
    RGFW_setGlobalHints_OpenGL(hints);

    RGFW_window *win = RGFW_createWindow("memplat", 0, 0, 256, 240,
                                         RGFW_windowEGL | RGFW_windowCenter | RGFW_windowNoResize);
    if (!win) {
        throw std::runtime_error("failed to create window");
    }
    RGFW_window_makeCurrentContext_EGL(win);
    RGFW_window_setExitKey(win, RGFW_keyEscape);

    return win;
}

void loadLevel(const char *path) {
    auto reader = ByteReader::open(path);
    auto parser = LevelParser(std::move(reader));
    parser.loadScreen();
}

int main(int argc, char **argv) {
    RGFW_window *win;
    try {
        win = initWindow();
        loadLevel("./data/levels/1-1.lvl");
    } catch (const std::exception &e) {
        fprintf(stderr, "startup failed: %s\n", e.what());
        return 1;
    }

    while (RGFW_window_shouldClose(win) == RGFW_FALSE) {
        RGFW_pollEvents();

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex2f(-0.6f, -0.75f);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex2f(0.6f, -0.75f);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex2f(0.0f, 0.75f);
        glEnd();
        RGFW_window_swapBuffers_EGL(win);
        glFlush();
    }

    RGFW_window_close(win);
    RGFW_deinit();

    return 0;
}

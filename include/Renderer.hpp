#ifndef RENDERER_H
#define RENDERER_H

#include "Viewport.hpp"
#include <cstdint>
#include <vector>

typedef struct RGFW_window RGFW_window;
typedef struct RGFW_surface RGFW_surface;

constexpr int WINDOW_WIDTH = 256;
constexpr int WINDOW_HEIGHT = 240;

RGFW_window *initWindow();

// pixels must stay alive for as long as surface is used, and outlive it.
RGFW_surface *createFrameSurface(RGFW_window *win, std::vector<uint8_t> &pixels);
void renderFrame(const Viewport &viewport, RGFW_window *win, RGFW_surface *surface,
                  std::vector<uint8_t> &pixels);
void freeSurface(RGFW_surface *surface);

// plain wrappers around RGFW functions
void pollEvents();
bool windowShouldClose(RGFW_window *win);
void closeWindow(RGFW_window *win);
void shutdownRenderer();

#endif // RENDERER_H

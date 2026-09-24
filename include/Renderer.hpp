#pragma once

#include <memory>

#include "Viewport.hpp"

struct wl_display;
struct wl_registry;
struct wl_compositor;
struct wl_shm;
struct wl_surface;
struct wl_callback;
struct wl_array;
struct wl_seat;
struct wl_keyboard;
struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;
struct wp_viewporter;
struct wp_viewport;

const int BUFFER_WIDTH = 256;
const int BUFFER_HEIGHT = 240;
const int VIEWPORT_TILE_SIZE = 16;

// owns a POSIX shared-memory framebuffer shared with the compositor
class ShmBuffer;
class Input;

// builds Wayland connection with the xdg_wm_base shell surface and a software
// wl_shm framebuffer; the compositor frame callbacks handle redraws, callers just need to
// keep calling 'dispatch()'
class Renderer {
  public:
    // the underlying buffer width and height are scaled up to match the targets here
    // the target dimensions currently need to be multiples of the buffer size
    Renderer(const char *title, const char *app_id, int target_width, int target_height,
             Viewport *game_viewport, Input *input);
    ~Renderer();

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    // represents a connection to the compositor, waiting until events have been queued then calling
    // handlers
    bool dispatch();

    // false if the shared-memory framebuffer couldn't be allocated; the renderer must not be
    // used further if so
    bool ok() const { return buffer != nullptr; }

    // wayland protocol callbacks take c function pointers; defines some simple thunks to satisfy
    // this from our renderer structs
    static void handleGlobal(void *data, wl_registry *registry, uint32_t name,
                             const char *interface, uint32_t version);
    static void handlePing(void *data, xdg_wm_base *base, uint32_t serial);
    static void handleSurfaceConfigure(void *data, xdg_surface *surface, uint32_t serial);
    static void handleToplevelClose(void *data, xdg_toplevel *toplevel);
    static void handleFrame(void *data, wl_callback *callback, uint32_t time_ms);
    static void handleSeat(void *data, wl_seat *seat, uint32_t capabilities);
    static void handleKeyboardLeave(void *data, wl_keyboard *keyboard, uint32_t serial,
                                    wl_surface *surface);
    static void handleKey(void *data, wl_keyboard *keyboard, uint32_t serial, uint32_t time,
                          uint32_t key, uint32_t state);

  private:
    void onGlobal(wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
    void onFrame(wl_callback *callback);
    void onSeat(wl_seat *seat, uint32_t capabilities);
    void createShellSurface(const char *title, const char *app_id);
    void redraw();

    wl_display *display = nullptr;
    wl_registry *registry = nullptr;
    wl_compositor *compositor = nullptr;
    wl_shm *shm = nullptr;
    wl_seat *seat = nullptr;
    wl_keyboard *keyboard = nullptr;
    xdg_wm_base *shell = nullptr;
    wp_viewporter *viewporter = nullptr;
    wp_viewport *viewport = nullptr;

    wl_surface *surface = nullptr;
    xdg_surface *shell_surface = nullptr;
    xdg_toplevel *toplevel = nullptr;
    wl_callback *frame_callback = nullptr;

    bool configured = false;
    bool closed = false;

    std::unique_ptr<ShmBuffer> buffer;
    Viewport *game_viewport = nullptr;
    Input *input = nullptr;
};

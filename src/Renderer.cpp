#include "Renderer.hpp"
#include "LevelParser.hpp"

#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <viewporter-client-protocol.h>
#include <xdg-shell-client-protocol.h>

namespace {

const wl_registry_listener registry_listener = {
    Renderer::handleGlobal,
    Renderer::handleGlobalRemove,
};

const xdg_wm_base_listener shell_listener = {Renderer::handlePing};

const xdg_surface_listener shell_surface_listener = {Renderer::handleSurfaceConfigure};

const xdg_toplevel_listener toplevel_listener = {Renderer::handleToplevelConfigure,
                                                 Renderer::handleToplevelClose};

const wl_callback_listener frame_listener = {Renderer::handleFrame};

} // namespace

// owns a POSIX shared-memory framebuffer shared with the compositor.
class ShmBuffer {
  public:
    ShmBuffer(wl_shm *shm, int width, int height);
    ~ShmBuffer();

    ShmBuffer(const ShmBuffer &) = delete;
    ShmBuffer &operator=(const ShmBuffer &) = delete;

    uint8_t *data() const { return static_cast<uint8_t *>(mapped); }
    wl_buffer *handle() const { return buffer; }
    std::size_t size() const { return byte_size; }
    // false if construction failed (logged to stderr); every other member function is
    // meaningless in that case
    bool valid() const { return buffer != nullptr; }

  private:
    // "/memplat-" + up to 10 digits of pid + '-' + up to 10 digits of instance_count + '\0'
    char shm_id[64];
    int fd = -1;
    // framebuffer data mem-mapped to a raw buffer
    void *mapped = nullptr;
    std::size_t byte_size = 0;
    wl_buffer *buffer = nullptr;
};

ShmBuffer::ShmBuffer(wl_shm *shm, int width, int height)
    : byte_size(static_cast<std::size_t>(width) * height * 2) {
    static int instance_count = 0;
    snprintf(shm_id, sizeof(shm_id), "/memplat-%d-%d", static_cast<int>(getpid()),
             instance_count++);

    fd = shm_open(shm_id, O_CREAT | O_RDWR | O_EXCL, 0600);
    if (fd < 0) {
        fprintf(stderr, "Failed to create shared memory buffer: %s\n", std::strerror(errno));
        return;
    }

    if (ftruncate(fd, static_cast<off_t>(byte_size)) != 0) {
        fprintf(stderr, "Failed to allocate buffer bytes: %s\n", std::strerror(errno));
        return;
    }

    mapped = mmap(nullptr, byte_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        mapped = nullptr;
        fprintf(stderr, "Failed to map buffer: %s\n", std::strerror(errno));
        return;
    }

    wl_shm_pool *pool = wl_shm_create_pool(shm, fd, static_cast<int32_t>(byte_size));
    buffer = wl_shm_pool_create_buffer(pool, 0, width, height, width * 2, WL_SHM_FORMAT_RGB565);
    wl_shm_pool_destroy(pool);
}

ShmBuffer::~ShmBuffer() {
    if (buffer) {
        wl_buffer_destroy(buffer);
    }
    if (mapped) {
        munmap(mapped, byte_size);
    }
    if (fd >= 0) {
        close(fd);
        shm_unlink(shm_id);
    }
}

Renderer::Renderer(const char *title, const char *app_id, int target_width, int target_height,
                   Viewport *game_viewport)
    : game_viewport(game_viewport) {
    display = wl_display_connect(nullptr);
    assert(display);

    registry = wl_display_get_registry(display);
    assert(registry);
    wl_registry_add_listener(registry, &registry_listener, this);

    // wait for a roundtrip so every registry global has been announced
    wl_display_roundtrip(display);

    assert(compositor);
    assert(shm);
    assert(shell);
    xdg_wm_base_add_listener(shell, &shell_listener, this);

    surface = wl_compositor_create_surface(compositor);
    assert(surface);

    assert(viewporter);
    viewport = wp_viewporter_get_viewport(viewporter, surface);

    createShellSurface(title, app_id);

    wp_viewport_set_destination(viewport, target_width, target_height);

    // xdg-wm-base expects a commit before attaching a buffer
    wl_surface_commit(surface);

    while (!configured) {
        wl_display_roundtrip(display);
    }

    buffer = std::make_unique<ShmBuffer>(shm, BUFFER_WIDTH, BUFFER_HEIGHT);
    if (!buffer->valid()) {
        buffer.reset();
        return;
    }
    std::memset(buffer->data(), 0xFF, buffer->size());

    wl_surface_attach(surface, buffer->handle(), 0, 0);

    redraw();
}

Renderer::~Renderer() {
    if (frame_callback) {
        wl_callback_destroy(frame_callback);
    }

    // destroy the buffer before the surface/shm it depends on
    buffer.reset();

    if (toplevel) {
        xdg_toplevel_destroy(toplevel);
    }
    if (shell_surface) {
        xdg_surface_destroy(shell_surface);
    }
    if (viewport) {
        wp_viewport_destroy(viewport);
    }
    if (viewporter) {
        wp_viewporter_destroy(viewporter);
    }
    if (surface) {
        wl_surface_destroy(surface);
    }
    if (shell) {
        xdg_wm_base_destroy(shell);
    }
    if (shm) {
        wl_shm_destroy(shm);
    }
    if (compositor) {
        wl_compositor_destroy(compositor);
    }
    if (registry) {
        wl_registry_destroy(registry);
    }
    if (display) {
        wl_display_flush(display);
        wl_display_disconnect(display);
    }
}

bool Renderer::dispatch() { return wl_display_dispatch(display) != -1 && !closed; }

void Renderer::createShellSurface(const char *title, const char *app_id) {
    shell_surface = xdg_wm_base_get_xdg_surface(shell, surface);
    assert(shell_surface);
    xdg_surface_add_listener(shell_surface, &shell_surface_listener, this);

    toplevel = xdg_surface_get_toplevel(shell_surface);
    assert(toplevel);
    xdg_toplevel_add_listener(toplevel, &toplevel_listener, this);
    xdg_toplevel_set_title(toplevel, title);
    xdg_toplevel_set_app_id(toplevel, app_id);

    // fixed-size, non-resizable window: min == max tells the compositor not
    // to tile/stretch us to fill a workspace
    xdg_toplevel_set_min_size(toplevel, BUFFER_WIDTH, BUFFER_HEIGHT);
    xdg_toplevel_set_max_size(toplevel, BUFFER_WIDTH, BUFFER_HEIGHT);
}

void Renderer::redraw() {
    for (int row = 0; row < BUFFER_HEIGHT / VIEWPORT_TILE_SIZE; ++row) {
        // load one col extra to partially display it; fine offset handling
        for (int col = 0; col <= BUFFER_WIDTH / VIEWPORT_TILE_SIZE; ++col) {
            TileRef tile = game_viewport->tileAt(col, row);

            uint8_t r, g, b;
            if (tile == 0) {
                r = row * 10;
                g = 128;
                b = 235; // light blue
            } else {
                float shade = static_cast<float>(tile) / 15.0f;
                r = static_cast<uint8_t>((shade / 2.0f) * 255.0f);
                g = static_cast<uint8_t>(shade * 255.0f);
                b = 0; // shades of green
            }

            // RGB565: 5 bits red, 6 bits green, 5 bits blue
            uint16_t pixel = static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
            std::array<uint16_t, VIEWPORT_TILE_SIZE> tile_row;
            // currently tiles are solid colour blocks so this is acceptable
            tile_row.fill(pixel);

            int pixel_x = col * VIEWPORT_TILE_SIZE - game_viewport->fineOffset();
            int pixel_y = row * VIEWPORT_TILE_SIZE;

            int clipped_x = std::max(pixel_x, 0);
            int visible_width = std::min(pixel_x + VIEWPORT_TILE_SIZE, BUFFER_WIDTH) - clipped_x;
            if (visible_width <= 0) {
                continue;
            }
            int src_x = clipped_x - pixel_x;

            for (int ty = 0; ty < VIEWPORT_TILE_SIZE; ++ty) {
                // x 2 to map from pixels to bytes
                std::size_t dest_offset = ((pixel_y + ty) * BUFFER_WIDTH + clipped_x) * 2;
                std::memcpy(buffer->data() + dest_offset, tile_row.data() + src_x,
                            static_cast<std::size_t>(visible_width) * 2);
            }
        }
    }

    wl_surface_attach(surface, buffer->handle(), 0, 0);
    wl_surface_damage_buffer(surface, 0, 0, BUFFER_WIDTH, BUFFER_HEIGHT);

    frame_callback = wl_surface_frame(surface);
    wl_callback_add_listener(frame_callback, &frame_listener, this);

    wl_surface_commit(surface);
}

// Wayland protocol callbacks, and corresponding thunks

void Renderer::onGlobal(wl_registry *registry, uint32_t name, const char *interface,
                        uint32_t version) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        // wl_surface_damage_buffer() requires wl_surface (and thus wl_compositor) version 4+
        uint32_t bind_version = version < 4 ? version : 4;
        compositor = static_cast<wl_compositor *>(
            wl_registry_bind(registry, name, &wl_compositor_interface, bind_version));
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        shm = static_cast<wl_shm *>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        shell =
            static_cast<xdg_wm_base *>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
    } else if (strcmp(interface, wp_viewporter_interface.name) == 0) {
        viewporter = static_cast<wp_viewporter *>(
            wl_registry_bind(registry, name, &wp_viewporter_interface, 1));
    }
}

void Renderer::handleGlobal(void *data, wl_registry *registry, uint32_t name, const char *interface,
                            uint32_t version) {
    static_cast<Renderer *>(data)->onGlobal(registry, name, interface, version);
}

void Renderer::handleGlobalRemove(void *, wl_registry *, uint32_t) {}

void Renderer::handlePing(void *, xdg_wm_base *base, uint32_t serial) {
    xdg_wm_base_pong(base, serial);
}

void Renderer::handleSurfaceConfigure(void *data, xdg_surface *surface, uint32_t serial) {
    static_cast<Renderer *>(data)->configured = true;
    xdg_surface_ack_configure(surface, serial);
}

void Renderer::handleToplevelConfigure(void *, xdg_toplevel *, int32_t, int32_t, wl_array *) {}

void Renderer::handleToplevelClose(void *data, xdg_toplevel *) {
    static_cast<Renderer *>(data)->closed = true;
}

void Renderer::handleFrame(void *data, wl_callback *callback, uint32_t) {
    Renderer *self = static_cast<Renderer *>(data);

    wl_callback_destroy(callback);
    self->frame_callback = nullptr;

    self->redraw();
}

#include "Platform.hpp"
#include "Fatal.hpp"
#include "Input.hpp"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <viewporter-client-protocol.h>
#include <wayland-client-protocol.h>
#include <xdg-shell-client-protocol.h>

namespace {

// we dont use these, but wayland still wants us to define them
void ignoreGlobalRemove(void *, wl_registry *, uint32_t) {}
void ignoreToplevelConfigure(void *, xdg_toplevel *, int32_t, int32_t, wl_array *) {}
void ignoreKeymap(void *, wl_keyboard *, uint32_t, int32_t, uint32_t) {}
void ignoreKeyboardEnter(void *, wl_keyboard *, uint32_t, wl_surface *, wl_array *) {}
void ignoreModifiers(void *, wl_keyboard *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {}
void ignoreRepeatInfo(void *, wl_keyboard *, int32_t, int32_t) {}

const wl_registry_listener registry_listener = {
    .global = Platform::handleGlobal,
    .global_remove = ignoreGlobalRemove,
};

const xdg_wm_base_listener shell_listener = {
    .ping = Platform::handlePing,
};

const xdg_surface_listener shell_surface_listener = {
    .configure = Platform::handleSurfaceConfigure,
};

const xdg_toplevel_listener toplevel_listener = {
    .configure = ignoreToplevelConfigure,
    .close = Platform::handleToplevelClose,
};

const wl_callback_listener frame_listener = {
    .done = Platform::handleFrame,
};

const wl_keyboard_listener keyboard_listener = {
    .keymap = ignoreKeymap,
    .enter = ignoreKeyboardEnter,
    .leave = Platform::handleKeyboardLeave,
    .key = Platform::handleKey,
    .modifiers = ignoreModifiers,
    .repeat_info = ignoreRepeatInfo,
};

const wl_seat_listener seat_listener = {
    .capabilities = Platform::handleSeat,
};

} // namespace

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
    shm_unlink(shm_id);

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
    }
}

Platform::Platform(const char *title, const char *app_id, int target_width, int target_height,
                   Input *input, Ticker *ticker, FrameSource *frame_source)
    : input(input), ticker(ticker), frame_source(frame_source) {
    assert(ticker);
    assert(frame_source);
    display = wl_display_connect(nullptr);
    if (!display) {
        fatal("could not connect to a Wayland display");
    }

    registry = wl_display_get_registry(display);
    assert(registry);
    wl_registry_add_listener(registry, &registry_listener, this);
    wl_display_roundtrip(display);

    if (!compositor || !shm || !shell || !viewporter) {
        fatal("compositor does not provide required components");
    }

    xdg_wm_base_add_listener(shell, &shell_listener, this);

    surface = wl_compositor_create_surface(compositor);
    assert(surface);

    viewport = wp_viewporter_get_viewport(viewporter, surface);

    if (seat) {
        wl_seat_add_listener(seat, &seat_listener, this);
    }

    createShellSurface(title, app_id, target_width, target_height);

    wp_viewport_set_destination(viewport, target_width, target_height);

    // xdg-wm-base expects a commit before attaching a buffer
    wl_surface_commit(surface);

    while (!configured) {
        if (wl_display_roundtrip(display) == -1) {
            fatal("lost connection to compositor");
        }
    }

    buffer = std::make_unique<ShmBuffer>(shm, BUFFER_WIDTH, BUFFER_HEIGHT);
    if (!buffer->valid()) {
        buffer.reset();
        fatal("failed to allocate framebuffer");
    }
    std::memset(buffer->data(), 0xFF, buffer->size());

    wl_surface_attach(surface, buffer->handle(), 0, 0);

    redraw();
}

Platform::~Platform() {
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
    if (seat) {
        wl_seat_destroy(seat);
    }
    if (keyboard) {
        wl_keyboard_destroy(keyboard);
    }
    if (display) {
        wl_display_flush(display);
        wl_display_disconnect(display);
    }
}

bool Platform::dispatch() { return wl_display_dispatch(display) != -1 && !closed; }

void Platform::createShellSurface(const char *title, const char *app_id, int target_width,
                                  int target_height) {
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
    xdg_toplevel_set_min_size(toplevel, target_width, target_height);
    xdg_toplevel_set_max_size(toplevel, target_width, target_height);
}

void Platform::redraw() {
    frame_source->render(reinterpret_cast<uint16_t *>(buffer->data()), BUFFER_WIDTH, BUFFER_HEIGHT);

    wl_surface_attach(surface, buffer->handle(), 0, 0);
    wl_surface_damage_buffer(surface, 0, 0, BUFFER_WIDTH, BUFFER_HEIGHT);

    frame_callback = wl_surface_frame(surface);
    wl_callback_add_listener(frame_callback, &frame_listener, this);

    wl_surface_commit(surface);
}

void Platform::tick() {
    ticker->tick();

    input->beginNextFrame();
}

// wayland protocol callbacks, and corresponding thunks

void Platform::onGlobal(wl_registry *registry, uint32_t name, const char *interface,
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
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        seat = static_cast<wl_seat *>(wl_registry_bind(registry, name, &wl_seat_interface, 1));
    }
}

void Platform::handleGlobal(void *data, wl_registry *registry, uint32_t name, const char *interface,
                            uint32_t version) {
    static_cast<Platform *>(data)->onGlobal(registry, name, interface, version);
}

void Platform::handlePing(void *, xdg_wm_base *base, uint32_t serial) {
    xdg_wm_base_pong(base, serial);
}

void Platform::handleSurfaceConfigure(void *data, xdg_surface *surface, uint32_t serial) {
    static_cast<Platform *>(data)->configured = true;
    xdg_surface_ack_configure(surface, serial);
}

void Platform::handleToplevelClose(void *data, xdg_toplevel *) {
    static_cast<Platform *>(data)->closed = true;
}

void Platform::onFrame(wl_callback *callback, uint32_t time_ms) {
    wl_callback_destroy(callback);
    frame_callback = nullptr;

    if (has_last_frame_time) {
        uint32_t delta = time_ms - last_frame_time_ms;
        // clamp so a stall won't create a large backlog
        elapsed_ms += delta < MAX_FRAME_MS ? delta : MAX_FRAME_MS;
    } else {
        has_last_frame_time = true;
    }
    last_frame_time_ms = time_ms;

    uint32_t ticks_due = (static_cast<uint64_t>(elapsed_ms) * TICK_HZ) / 1000;
    while (ticks_run < ticks_due) {
        tick();
        ++ticks_run;
    }

    redraw();
}

void Platform::handleFrame(void *data, wl_callback *callback, uint32_t time_ms) {
    static_cast<Platform *>(data)->onFrame(callback, time_ms);
}

void Platform::onSeat(wl_seat *seat, uint32_t capabilities) {
    if (!keyboard && capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(keyboard, &keyboard_listener, this);
    }
}

void Platform::handleSeat(void *data, wl_seat *seat, uint32_t capabilities) {
    static_cast<Platform *>(data)->onSeat(seat, capabilities);
}

void Platform::handleKeyboardLeave(void *data, wl_keyboard *, uint32_t, wl_surface *) {
    static_cast<Platform *>(data)->input->onFocusLost();
}

void Platform::handleKey(void *data, wl_keyboard *, uint32_t, uint32_t, uint32_t key,
                         uint32_t state) {
    static_cast<Platform *>(data)->input->onKey(key, state);
}

cmake_minimum_required(VERSION 3.10)

project(
    memplat
    VERSION 1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)

# wayland-client.pc lists "-lm" in its Libs, but libwayland-client itself has no NEEDED
# entry for libm (readelf -d confirms) and nothing in this project calls a math function;
# drop it so we don't map in a whole extra shared library for nothing.
list(REMOVE_ITEM WAYLAND_CLIENT_LIBRARIES m)
pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner REQUIRED)

set(GENERATED_PROTOCOL_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_PROTOCOL_DIR})

# Generates <name>-client-protocol.h/.c from a Wayland protocol XML file via
# wayland-scanner, and appends the .c to GENERATED_PROTOCOL_SOURCES.
function(generate_wayland_protocol xml_path name)
    set(header "${GENERATED_PROTOCOL_DIR}/${name}-client-protocol.h")
    set(source "${GENERATED_PROTOCOL_DIR}/${name}-client-protocol.c")

    add_custom_command(
        OUTPUT ${header}
        COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header ${xml_path} ${header}
        DEPENDS ${xml_path}
        COMMENT "wayland-scanner: generating ${name}-client-protocol.h"
    )
    add_custom_command(
        OUTPUT ${source}
        COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code ${xml_path} ${source}
        DEPENDS ${xml_path} ${header}
        COMMENT "wayland-scanner: generating ${name}-client-protocol.c"
    )

    set(GENERATED_PROTOCOL_SOURCES ${GENERATED_PROTOCOL_SOURCES} ${source} PARENT_SCOPE)
endfunction()

generate_wayland_protocol(${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml xdg-shell)
generate_wayland_protocol(${WAYLAND_PROTOCOLS_DIR}/stable/viewporter/viewporter.xml viewporter)

set(
    SOURCE_FILES
    src/Main.cpp
    src/LevelParser.cpp
    src/ByteReader.cpp
    src/Viewport.cpp
    src/Renderer.cpp
    ${GENERATED_PROTOCOL_SOURCES}
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_compile_options(${PROJECT_NAME} PRIVATE
    -ffunction-sections -fdata-sections
    # no ASLR/unwind support needed: this is a single-purpose local binary that never throws
    # or unwinds (see -fno-exceptions below), so both cost real bytes for no benefit here
    -fno-pie -fno-asynchronous-unwind-tables -fno-unwind-tables
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti -fno-exceptions>
)
target_link_options(${PROJECT_NAME} PRIVATE
    -Wl,--gc-sections -s
    -Wl,-z,noseparate-code
    -Wl,--build-id=none
    -Wl,--as-needed
    -no-pie
    -static-libgcc
    -static-libstdc++
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${WAYLAND_CLIENT_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${WAYLAND_CLIENT_INCLUDE_DIRS})

target_include_directories(${PROJECT_NAME} PRIVATE include ${GENERATED_PROTOCOL_DIR})

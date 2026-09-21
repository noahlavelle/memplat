cmake_minimum_required(VERSION 3.10)

project(
    memplat
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(
    SOURCE_FILES
    src/Main.cpp
    src/LevelParser.cpp
    src/ByteReader.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_compile_options(${PROJECT_NAME} PRIVATE
    -ffunction-sections -fdata-sections
    -fno-rtti
)
target_link_options(${PROJECT_NAME} PRIVATE
    -Wl,--gc-sections -s
    -Wl,-z,noseparate-code
    -Wl,--build-id=none
)

target_link_libraries(${PROJECT_NAME} PRIVATE X11 Xrandr GL m)

target_include_directories(${PROJECT_NAME} PRIVATE include)

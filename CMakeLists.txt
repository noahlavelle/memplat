cmake_minimum_required(VERSION 3.10)

project(
    memplat
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(
    SOURCE_FILES
    src/Main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

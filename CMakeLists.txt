cmake_minimum_required(VERSION 3.1)

project(rasterizePolygon
    VERSION 1.0.0
    DESCRIPTION "library for rasterizing 2d shapes"
    LANGUAGES CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(SOURCES main.cpp)
set(LIBRARY tgaimage.cpp)
add_library(lib STATIC ${LIBRARY})
include_directories({PROJECT_SOURCE_DIR})

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} lib)
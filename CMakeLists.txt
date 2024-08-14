cmake_minimum_required(VERSION 3.1)

project(rasterizePolygon
    VERSION 1.0.0
    DESCRIPTION "library for rasterizing 2d shapes"
    LANGUAGES CXX)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# find_package( OpenCV PATHS "include/build/" )
find_package(OpenCV REQUIRED PATHS "include/" NO_DEFAULT_PATH) 
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found. Please make sure it is installed and CMake can find it.")
endif()

include_directories(${OpenCV_INCLUDE_DIRS})
set(SOURCES main.cpp)
set(LIBRARY tgaimage.cpp)

# Создаем библиотеку
add_library(lib SHARED ${LIBRARY})

# Создаем исполняемый файл
add_executable(${PROJECT_NAME} ${SOURCES})

# Линкуем библиотеку к исполняемому файлу
target_link_libraries(${PROJECT_NAME} lib ${OpenCV_LIBS})
cmake_minimum_required(VERSION 2.6)
project(mapgen)

include_directories(./src/)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FALGS} -O3 -g -std=c++20 -pthread")

add_executable(mapgen 
    src/helper.cpp 
    src/generators/shapes.cpp 
    src/generators/noise.cpp 
    src/generators/terrain.cpp 
    src/generators/water.cpp 
    src/generators/moisture.cpp 
    src/generators/temperature.cpp 
    src/generators/biome.cpp 
    src/main.cpp 

    src/circle_stack.cpp
)

install(TARGETS mapgen RUNTIME DESTINATION bin)

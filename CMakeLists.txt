cmake_minimum_required(VERSION 2.6)
project(mapgen)

include_directories(../network/)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FALGS} -O3 -g -std=c++20 -pthread")

add_executable(mapgen 
    src/helper.cpp 
    src/generate_terrain.cpp 
    src/main.cpp 

    src/temperature_generator.cpp
    src/moisture_generator.cpp
    src/biome_generator.cpp
    src/circle_stack.cpp
)

install(TARGETS mapgen RUNTIME DESTINATION bin)

cmake_minimum_required(VERSION 2.6)
project(mapgen)

include_directories(../network/)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FALGS} -O3 -std=c++17 -pthread")

add_executable(mapgen src/main.cpp src/generate_terrain.cpp src/generate_water.cpp src/merge_map.cpp src/helper.cpp)

install(TARGETS mapgen RUNTIME DESTINATION bin)

cmake_minimum_required(VERSION 2.6)
project(mapgen)

include_directories(../network/)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FALGS} -O3 -std=c++11 -pthread")

add_executable(mapgen src/main.cpp src/generate_map.cpp)

install(TARGETS mapgen RUNTIME DESTINATION bin)

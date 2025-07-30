cmake_minimum_required(VERSION 3.31)
project(PolyStation)

set(CMAKE_CXX_STANDARD 20)

add_executable(PolyStation src/main.cpp
        src/bios.cpp
        src/bus.cpp
        src/cpu.cpp)

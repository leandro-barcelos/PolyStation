cmake_minimum_required(VERSION 3.31)
project(PolyStation)

set(CMAKE_CXX_STANDARD 20)

find_package(Microsoft.GSL CONFIG REQUIRED)

add_executable(PolyStation src/main.cpp
        src/bios.cpp
        src/bus.cpp
        src/cpu.cpp)

target_link_libraries(PolyStation PRIVATE Microsoft.GSL::GSL)

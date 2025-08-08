cmake_minimum_required(VERSION 3.31)
project(PolyStation)

set(CMAKE_CXX_STANDARD 20)

find_package(Microsoft.GSL CONFIG REQUIRED)
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

# ImGUI
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)

set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(imgui PUBLIC
        Vulkan::Vulkan
        SDL2::SDL2
        SDL2::SDL2main
)

target_compile_definitions(imgui PUBLIC
        VK_USE_PLATFORM_XLIB_KHR
)

add_executable(PolyStation src/main.cpp
        src/bios.cpp
        src/bios.h
        src/bus.cpp
        src/bus.h
        src/cpu.cpp
        src/cpu.h
        src/app.cpp
        src/app.h
        src/ram.cpp
        src/ram.h)

target_link_libraries(PolyStation PRIVATE
        Microsoft.GSL::GSL
        imgui
        Vulkan::Vulkan
)

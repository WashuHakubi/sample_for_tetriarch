cmake_minimum_required(VERSION 3.28)
project(imgui)

file(GLOB imgui_FILES CONFIGURE_DEPENDS "*.cpp")

add_library(imgui STATIC
        ${imgui_FILES}
        backends/imgui_impl_sdlgpu3.cpp
        backends/imgui_impl_sdlgpu3.h
        backends/imgui_impl_sdl3.cpp
        backends/imgui_impl_sdl3.h
        misc/cpp/imgui_stdlib.cpp
        misc/cpp/imgui_stdlib.h
)

target_include_directories(imgui PUBLIC .)
target_link_libraries(imgui SDL3::SDL3)
file(GLOB_RECURSE client_FILES CONFIGURE_DEPENDS "src/**.cpp")

add_executable(client ${client_FILES})

target_link_libraries(client
        SDL3::SDL3
        bgfx::bgfx
        EnTT::EnTT
        nlohmann_json::nlohmann_json
        imgui
        freetype
        shared)

target_compile_definitions(client PUBLIC SDL_MAIN_USE_CALLBACKS)

if (MSVC)
    target_compile_options(client PRIVATE /W4 /WX /wd4100 /wd4996)
else ()
    target_compile_options(client PRIVATE
            -Wall
            -Wextra
            -Werror
            -Wno-unused-parameter
            -Wno-unused-function
            -Wno-unused-private-field
            -Wno-missing-field-initializers)
endif ()

bgfx_compile_shaders(
        TYPE VERTEX
        VARYING_DEF ${PROJECT_SOURCE_DIR}/assets/shaders/varying.def.sc
        OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/shaders
        OUT_FILES_VAR out_vertex_shaders
        SHADERS ${PROJECT_SOURCE_DIR}/assets/shaders/vs_cubes.sc
)

bgfx_compile_shaders(
        TYPE FRAGMENT
        VARYING_DEF ${PROJECT_SOURCE_DIR}/assets/shaders/varying.def.sc
        OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/shaders
        OUT_FILES_VAR out_fragment_shaders
        SHADERS ${PROJECT_SOURCE_DIR}/assets/shaders/fs_cubes.sc
)

target_sources(client PRIVATE ${out_fragment_shaders} ${out_vertex_shaders})

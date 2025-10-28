file(GLOB_RECURSE client_FILES CONFIGURE_DEPENDS "src/**.cpp")

add_executable(client ${client_FILES})

target_link_libraries(client
        SDL3::SDL3
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
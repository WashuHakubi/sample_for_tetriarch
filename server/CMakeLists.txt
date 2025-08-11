file(GLOB_RECURSE server_FILES CONFIGURE_DEPENDS "src/**.cpp")

add_executable(server ${server_FILES})
target_link_libraries(server
        shared)
target_include_directories(server PRIVATE src)

if (MSVC)
    target_compile_options(server PRIVATE /W4 /WX /wd4100 /wd4996)
else ()
    target_compile_options(server PRIVATE
            -Wall
            -Wextra
            -Werror
            -Wno-unused-parameter
            -Wno-unused-function
            -Wno-unused-private-field
            -Wno-missing-field-initializers)
endif ()
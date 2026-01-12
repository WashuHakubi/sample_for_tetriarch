file(GLOB_RECURSE server_lib_FILES CONFIGURE_DEPENDS "src/**.cpp")
file(GLOB_RECURSE server_FILES CONFIGURE_DEPENDS "app/**.cpp")
file(GLOB_RECURSE server_tests_FILES CONFIGURE_DEPENDS "tests/**.cpp")

add_library(server_lib STATIC ${server_lib_FILES})
target_link_libraries(server_lib PUBLIC
        shared
        libuv::uv
)
target_include_directories(server_lib PUBLIC include)

if(MSVC)
        target_compile_options(server_lib PRIVATE /W4 /WX)
else()
        target_compile_options(server_lib PRIVATE -Wall -Wextra -Wpedantic -Werror
        )
endif()

add_executable(server ${server_FILES})
target_link_libraries(server server_lib)

if(MSVC)
        target_compile_options(server PRIVATE /W4 /WX)
else()
        target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic -Werror
        )
endif()

add_executable(server_tests ${server_tests_FILES})
target_link_libraries(server_tests server_lib Catch2::Catch2WithMain)
target_include_directories(server_tests PRIVATE tests)


if(MSVC)
        target_compile_options(server_tests PRIVATE /W4 /WX)
else()
        target_compile_options(server_tests PRIVATE -Wall -Wextra -Wpedantic -Werror
        )
endif()

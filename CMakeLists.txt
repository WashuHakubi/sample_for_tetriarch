cmake_minimum_required(VERSION 3.28)
project(test)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)
FetchContent_Declare(FetchDependency
  GIT_REPOSITORY https://github.com/jpetrie/fetch-dependency.git
  GIT_TAG origin/main
)
FetchContent_MakeAvailable(FetchDependency)
include(${fetchdependency_SOURCE_DIR}/FetchDependency.cmake)

# fetch_dependency(Catch2
#         GIT_REPOSITORY https://github.com/catchorg/Catch2.git
#         GIT_TAG v3.8.0
#         CONFIGURATION ${CMAKE_BUILD_TYPE}
# )

fetch_dependency(
        concurrencpp
        GIT_REPOSITORY https://github.com/WashuHakubi/concurrencpp.git
        GIT_TAG master
        CONFIGURATION ${CMAKE_BUILD_TYPE}
)

fetch_dependency(
        ryml
        GIT_REPOSITORY https://github.com/biojppm/rapidyaml.git
        GIT_TAG v0.7.2
        CONFIGURATION ${CMAKE_BUILD_TYPE}
)

fetch_dependency(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.4
        CONFIGURATION ${CMAKE_BUILD_TYPE}
)

# fetch_dependency(
#         SDL3_ttf
#         GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
#         # Update when SDL3_ttf is properly released
#         GIT_TAG 51adc815b2959dd918c5c3970eb05d87dc0b5e56
#         CONFIGURATION ${CMAKE_BUILD_TYPE}
# )

fetch_dependency(
        crossguid
        GIT_REPOSITORY https://github.com/graeme-hill/crossguid.git
        GIT_TAG master
        CONFIGURATION ${CMAKE_BUILD_TYPE}
)

SET(SDLSHADERCROSS_VENDORED ON)
add_subdirectory(external/SDL_shadercross)

add_subdirectory(imgui)

file(GLOB_RECURSE test_FILES CONFIGURE_DEPENDS "src/**.cpp")
add_executable(test
        ${test_FILES}
)

target_link_libraries(test
        crossguid
        concurrencpp::concurrencpp
        ryml::ryml
        SDL3::SDL3
        # SDL3_ttf::SDL3_ttf
        imgui
        SDL3_shadercross::SDL3_shadercross)

target_include_directories(test PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_definitions(test PUBLIC SDL_MAIN_USE_CALLBACKS)

if (CMAKE_HOST_WIN32)
        add_custom_command(TARGET test POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:test> $<TARGET_FILE_DIR:test>
                COMMAND_EXPAND_LISTS
        )
endif()

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets
)

add_dependencies(test copy_assets)


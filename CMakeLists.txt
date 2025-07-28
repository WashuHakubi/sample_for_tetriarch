cmake_minimum_required(VERSION 3.28)
project(test)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)
FetchContent_Declare(FetchDependency
        GIT_REPOSITORY https://github.com/jpetrie/fetch-dependency.git
        GIT_TAG origin/main
)
FetchContent_MakeAvailable(FetchDependency)
include(${fetchdependency_SOURCE_DIR}/FetchDependency.cmake)

fetch_dependency(Catch2
        GIT_SOURCE https://github.com/catchorg/Catch2.git
        VERSION v3.8.0
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        ROOT ${PROJECT_SOURCE_DIR}/ext
)

fetch_dependency(
        concurrencpp
        GIT_SOURCE https://github.com/WashuHakubi/concurrencpp.git
        VERSION origin/master
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        ROOT ${PROJECT_SOURCE_DIR}/ext
)

fetch_dependency(
        ryml
        GIT_SOURCE https://github.com/biojppm/rapidyaml.git
        VERSION v0.9.0
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        ROOT ${PROJECT_SOURCE_DIR}/ext
)

fetch_dependency(
        SDL3
        GIT_SOURCE https://github.com/libsdl-org/SDL.git
        VERSION release-3.2.10
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        CONFIGURE_OPTIONS -DSDL_SHARED=OFF
        -DSDL_STATIC=ON
        -DBUILD_SHARED_LIBS=OFF
        ROOT ${PROJECT_SOURCE_DIR}/ext
)

fetch_dependency(
        crossguid
        GIT_SOURCE https://github.com/WashuHakubi/crossguid.git
        VERSION origin/master
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        ROOT ${PROJECT_SOURCE_DIR}/ext
        CONFIGURE_OPTIONS -DCROSSGUID_TESTS=OFF
)

fetch_dependency(
        freetype
        GIT_SOURCE https://github.com/freetype/freetype.git
        VERSION VER-2-13-3
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        CONFIGURE_OPTIONS
        ROOT ${PROJECT_SOURCE_DIR}/ext
)

fetch_dependency(
        SDL3_image
        GIT_SOURCE https://github.com/libsdl-org/SDL_image.git
        VERSION release-3.2.4
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        CONFIGURE_OPTIONS -DENABLE_ARC=OFF
        -DBUILD_SHARED_LIBS=OFF
        ROOT ${PROJECT_SOURCE_DIR}/ext
)

fetch_dependency(
        RmlUi
        GIT_SOURCE https://github.com/mikke89/RmlUi.git
        VERSION cdff6ad9dcf04989275ef5c88f792c488f80a508 # Using 6.2 instead of 6.1 to fix Mac mouse positions
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        CONFIGURE_OPTIONS -DRMLUI_SAMPLES=OFF
        -DRMLUI_BACKEND=native
        -DRMLUI_FONT_ENGINE=freetype
        -DRMLUI_THIRDPARTY_CONTAINERS=OFF
        -DBUILD_SHARED_LIBS=OFF
        ROOT ${PROJECT_SOURCE_DIR}/ext
        OUT_SOURCE_DIR RML_OUT_DIR
)

fetch_dependency(
        ng-log
        GIT_SOURCE https://github.com/ng-log/ng-log.git
        VERSION v0.8.0
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        CONFIGURE_OPTIONS -DBUILD_SHARED_LIBS=OFF
        ROOT ${PROJECT_SOURCE_DIR}/ext
)

# Add the RmlUi SDL backend. This can be tweaked to support any of the SDL backends.
add_library(RmlSdlBackend STATIC
        ${RML_OUT_DIR}/Backends/RmlUi_Platform_SDL.h
        ${RML_OUT_DIR}/Backends/RmlUi_Platform_SDL.cpp
        ${RML_OUT_DIR}/Backends/RmlUi_Renderer_SDL.h
        ${RML_OUT_DIR}/Backends/RmlUi_Renderer_SDL.cpp)

target_include_directories(RmlSdlBackend PUBLIC
        ${RML_OUT_DIR}/Backends)

target_compile_definitions(RmlSdlBackend PUBLIC
        RMLUI_SDL_VERSION_MAJOR=3)

target_link_libraries(RmlSdlBackend PRIVATE
        RmlUi::RmlUi
        SDL3::SDL3
        SDL3_image::SDL3_image
)

add_subdirectory(ext)

add_subdirectory(shared)
add_subdirectory(client)
add_subdirectory(server)

#[[

file(GLOB_RECURSE sample_FILES CONFIGURE_DEPENDS "src/**.cpp")
add_executable(sample
        ${sample_FILES}
)

target_link_libraries(sample
        crossguid
        concurrencpp::concurrencpp
        ryml::ryml
        SDL3::SDL3
        imgui
        freetype
        RmlUi::RmlUi
        RmlSdlBackend
        ng-log::ng-log)

target_include_directories(sample PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_definitions(sample PUBLIC SDL_MAIN_USE_CALLBACKS)

if (MSVC)
    target_compile_options(sample PRIVATE /W4 /WX /wd4100 /wd4996)
else ()
    target_compile_options(sample PRIVATE
            -Wall
            -Wextra
            -Werror
            -Wno-unused-parameter
            -Wno-unused-function
            -Wno-unused-private-field
            -Wno-missing-field-initializers)
endif ()


if (CMAKE_HOST_WIN32)
    add_custom_command(TARGET sample POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:sample> $<TARGET_FILE_DIR:sample>
            COMMAND_EXPAND_LISTS
    )
endif ()

add_custom_target(copy_all_assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets
)

add_dependencies(sample copy_all_assets)]]

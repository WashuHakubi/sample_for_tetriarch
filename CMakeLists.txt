cmake_minimum_required(VERSION 3.28)
project(test)

if (CMAKE_BUILD_TYPE STREQUAL "")
    MESSAGE(WARNING "CMAKE_BUILD_TYPE not set. Defaulting to Debug")
    set(CMAKE_BUILD_TYPE Debug)
endif ()

if (MSVC)
    add_compile_options(/W4 /WX /wd4100 /wd4996)
else ()
    add_compile_options(
            -Wall
            -Wextra
            -Werror
            -Wno-unused-parameter
            -Wno-unused-function
            -Wno-unused-private-field
            -Wno-missing-field-initializers)
endif ()

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

fetch_dependency(libuv
        GIT_SOURCE https://github.com/libuv/libuv.git
        VERSION v1.51.0
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        ROOT ${PROJECT_SOURCE_DIR}/ext
)

fetch_dependency(nlohmann_json
        GIT_SOURCE https://github.com/nlohmann/json.git
        VERSION v3.11.3
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        ROOT ${PROJECT_SOURCE_DIR}/ext
)

fetch_dependency(glm
        GIT_SOURCE https://github.com/g-truc/glm.git
        VERSION 1.0.1
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        CONFIGURE_OPTIONS -DGLM_BUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF
        ROOT ${PROJECT_SOURCE_DIR}/ext
)

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
        # VERSION release-3.2.24
        VERSION origin/main
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        CONFIGURE_OPTIONS -DSDL_SHARED=OFF -DSDL_STATIC=ON -DBUILD_SHARED_LIBS=OFF
        ROOT ${PROJECT_SOURCE_DIR}/ext
)

fetch_dependency(
        freetype
        GIT_SOURCE https://github.com/freetype/freetype.git
        VERSION VER-2-13-3
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        ROOT ${PROJECT_SOURCE_DIR}/ext
)

fetch_dependency(
        SDL3_image
        GIT_SOURCE https://github.com/libsdl-org/SDL_image.git
        VERSION release-3.2.4
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        CONFIGURE_OPTIONS -DENABLE_ARC=OFF -DBUILD_SHARED_LIBS=OFF
        ROOT ${PROJECT_SOURCE_DIR}/ext
)

fetch_dependency(
        SDL3_shadercross
        GIT_SOURCE https://github.com/libsdl-org/SDL_shadercross.git
        VERSION origin/main
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        CONFIGURE_OPTIONS -DBUILD_SHARED_LIBS=OFF -DSDLSHADERCROSS_VENDORED=ON -DSDLSHADERCROSS_INSTALL=ON
        ROOT ${PROJECT_SOURCE_DIR}/ext
)

fetch_dependency(
        gflags
        GIT_SOURCE https://github.com/gflags/gflags.git
        VERSION v2.2.2
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        CONFIGURE_OPTIONS -DBUILD_SHARED_LIBS=OFF -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        ROOT ${PROJECT_SOURCE_DIR}/ext
)

fetch_dependency(
        ng-log
        GIT_SOURCE https://github.com/ng-log/ng-log.git
        VERSION v0.8.0
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        CONFIGURE_OPTIONS -DBUILD_SHARED_LIBS=OFF
        ROOT ${PROJECT_SOURCE_DIR}/ext
)

# fetch_dependency(
#        RmlUi
#        GIT_SOURCE https://github.com/mikke89/RmlUi.git
#        VERSION cdff6ad9dcf04989275ef5c88f792c488f80a508 # Using 6.2 instead of 6.1 to fix Mac mouse positions
#        CONFIGURATION ${CMAKE_BUILD_TYPE}
#        CONFIGURE_OPTIONS -DRMLUI_SAMPLES=OFF -DRMLUI_BACKEND=native -DRMLUI_FONT_ENGINE=freetype -DRMLUI_THIRDPARTY_CONTAINERS=OFF -DBUILD_SHARED_LIBS=OFF
#        ROOT ${PROJECT_SOURCE_DIR}/ext
#        OUT_SOURCE_DIR RML_OUT_DIR
# )
# # Add the RmlUi SDL backend. This can be tweaked to support any of the SDL backends.
# add_library(RmlSdlBackend STATIC
#        ${RML_OUT_DIR}/Backends/RmlUi_Platform_SDL.h
#        ${RML_OUT_DIR}/Backends/RmlUi_Platform_SDL.cpp
#        ${RML_OUT_DIR}/Backends/RmlUi_Renderer_SDL.h
#        ${RML_OUT_DIR}/Backends/RmlUi_Renderer_SDL.cpp)

# target_include_directories(RmlSdlBackend PUBLIC
#        ${RML_OUT_DIR}/Backends)

# target_compile_definitions(RmlSdlBackend PUBLIC
#        RMLUI_SDL_VERSION_MAJOR=3)

# target_link_libraries(RmlSdlBackend PRIVATE
#        RmlUi::RmlUi
#        SDL3::SDL3
#        SDL3_image::SDL3_image
# )

add_subdirectory(ext)

add_subdirectory(shared)
add_subdirectory(client)
add_subdirectory(server)

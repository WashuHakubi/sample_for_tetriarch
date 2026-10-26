cmake_minimum_required(VERSION 3.22)

project(piece-of-dim)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

include(FetchContent)
FetchContent_Declare(FetchDependency
  GIT_REPOSITORY https://github.com/jpetrie/fetch-dependency.git
  GIT_TAG 2.0.0
)
FetchContent_MakeAvailable(FetchDependency)
include(${fetchdependency_SOURCE_DIR}/FetchDependency.cmake)

# Unit test framework
fetch_dependency(Catch2
  GIT_SOURCE https://github.com/catchorg/Catch2.git
  VERSION 08092139210881f01a34e60c5f2fd6afa0ea4024
  CONFIGURATION ${CMAKE_BUILD_TYPE}
  OUT_SOURCE_DIR catch2_src_dir
)

# GLTF loading framework
fetch_dependency(fastgltf
  GIT_SOURCE https://github.com/spnda/fastgltf.git
  VERSION v0.9.1
  CONFIGURATION ${CMAKE_BUILD_TYPE}
  CONFIGURE_OPTIONS
    "-DCMAKE_CXX_STANDARD=20"
    "-DFASTGLTF_ENABLE_EXAMPLES=OFF"
    "-DFASTGLTF_ENABLE_TESTS=OFF"
)

# Math library
fetch_dependency(glm
  GIT_SOURCE https://github.com/g-truc/glm.git
  VERSION 1.0.3
  CONFIGURATION ${CMAKE_BUILD_TYPE}
)

fetch_dependency(lua
  GIT_SOURCE https://github.com/marovira/lua.git
  VERSION "3b22ffbd2d8b864eef0e7c8333dd18066d6ea063"
  CONFIGURATION ${CMAKE_BUILD_TYPE}
  CONFIGURE_OPTIONS "-DLUA_INSTALL_TARGET=ON"
)

fetch_dependency(nlohmann_json
  GIT_SOURCE https://github.com/nlohmann/json.git
  VERSION v3.12.0
  CONFIGURATION ${CMAKE_BUILD_TYPE}
)

# Windowing/input library
fetch_dependency(SDL3
  GIT_SOURCE https://github.com/libsdl-org/SDL.git
  VERSION release-3.4.18
  CONFIGURATION ${CMAKE_BUILD_TYPE}
)

find_package(SDL3 REQUIRED)

fetch_dependency(SDL3_shadercross
        GIT_SOURCE https://github.com/libsdl-org/SDL_shadercross.git
        VERSION origin/main
        CONFIGURATION ${CMAKE_BUILD_TYPE}
        CONFIGURE_OPTIONS
            "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}/bin"
            "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}/bin"
            "-DSDLSHADERCROSS_VENDORED=ON"
            "-DSPIRV_SKIP_TESTS=ON"
            "-DCMAKE_PREFIX_PATH=${SDL3_DIR}"
)

# Json parsing library
fetch_dependency(simdjson
  GIT_SOURCE https://github.com/simdjson/simdjson.git
  VERSION v5.0.4
  CONFIGURATION ${CMAKE_BUILD_TYPE}
  CONFIGURE_OPTIONS "-DCMAKE_CXX_STANDARD=20"
)

# Fast UTF8 parsing (for fastgltf)
fetch_dependency(simdutf
  GIT_SOURCE https://github.com/simdutf/simdutf.git
  VERSION v9.2.1
  CONFIGURATION ${CMAKE_BUILD_TYPE}
  CONFIGURE_OPTIONS "-DCMAKE_CXX_STANDARD=20"
)

# Logging
fetch_dependency(spdlog
  GIT_SOURCE https://github.com/gabime/spdlog
  VERSION v1.17.0
  CONFIGURATION ${CMAKE_BUILD_TYPE}
)

fetch_dependency(stb
  GIT_SOURCE https://github.com/nothings/stb.git
  VERSION origin/master
  NO_BUILD
  OUT_SOURCE_DIR stb_lib_dir
)

# Coroutine/reflection support library
fetch_dependency(wx
  GIT_SOURCE https://github.com/WashuHakubi/wx.git
  VERSION origin/main
  CONFIGURATION ${CMAKE_BUILD_TYPE}
  CONFIGURE_OPTIONS "-DWX_BUILD_TESTS=OFF"
)


include(CTest)
include("${catch2_src_dir}/extras/Catch.cmake")

add_subdirectory(game)

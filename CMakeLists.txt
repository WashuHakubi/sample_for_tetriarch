cmake_minimum_required(VERSION 3.28)
project(test)

if(CMAKE_BUILD_TYPE STREQUAL "")
  message(WARNING "CMAKE_BUILD_TYPE not set. Defaulting to Debug")
  set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_definitions(GLM_FORCE_LEFT_HANDED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

set(CMAKE_CXX_STANDARD 20)

include(ext/bgfx.cmake/cmake/bgfxToolUtils.cmake)

add_subdirectory(ext)

add_subdirectory(engine)
add_subdirectory(game)
add_subdirectory(test)

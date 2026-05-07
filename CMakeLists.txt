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

find_package(Git)
message(STATUS ${PROJECT_SOURCE_DIR})
execute_process(
    COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
    COMMAND_ECHO STDOUT
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMAND_ERROR_IS_FATAL ANY)

add_subdirectory(third-party)

add_subdirectory(game)

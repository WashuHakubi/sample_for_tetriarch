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

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
  set(DOTNET_SYSTEM_NAME "linux")
  set(DOTNET_OS_ARCH "x64")
  set(DOTNET_FILE_EXT "tar.gz")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
  set(DOTNET_SYSTEM_NAME "osx")
  set(DOTNET_OS_ARCH "arm64")
  set(DOTNET_FILE_EXT "tar.gz")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
  set(DOTNET_SYSTEM_NAME "win")
  set(DOTNET_OS_ARCH "x64")
  set(DOTNET_FILE_EXT "zip")
else()
  message(FATAL_ERROR "Unknown host system ${CMAKE_HOST_SYSTEM_NAME}")
endif()

set(DOTNET_VERSION "10.0.203")
set(DOTNET_FILE_NAME
    "dotnet-sdk-${DOTNET_VERSION}-${DOTNET_SYSTEM_NAME}-${DOTNET_OS_ARCH}.${DOTNET_FILE_EXT}")
set(DOTNET_FILE_PATH "${PROJECT_SOURCE_DIR}/third-party/${DOTNET_FILE_NAME}")
set(DOTNET_PATH "${PROJECT_SOURCE_DIR}/third-party/dotnet")

if(NOT EXISTS ${DOTNET_FILE_PATH})
  set(DOTNET_URL
      "https://builds.dotnet.microsoft.com/dotnet/Sdk/${DOTNET_VERSION}/${DOTNET_FILE_NAME}")
  file(DOWNLOAD ${DOTNET_URL} ${DOTNET_FILE_PATH})

  file(ARCHIVE_EXTRACT INPUT ${DOTNET_FILE_PATH} DESTINATION ${DOTNET_PATH})
endif()

set(DOTNET_HOST_PATH
    "${DOTNET_PATH}/packs/Microsoft.NETCore.App.Host.${DOTNET_SYSTEM_NAME}-${DOTNET_OS_ARCH}/10.0.7/runtimes/${DOTNET_SYSTEM_NAME}-${DOTNET_OS_ARCH}/native")

add_subdirectory(third-party)

add_subdirectory(game)

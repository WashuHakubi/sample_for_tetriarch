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

fetch_dependency(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.8.0
        CONFIGURATION ${CMAKE_BUILD_TYPE}
)

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

file(GLOB_RECURSE ENGINE_FILES CONFIGURE_DEPENDS "engine/*.cpp")
file(GLOB_RECURSE GAME_FILES CONFIGURE_DEPENDS "game/*.cpp")

add_executable(test
        main.cpp
        ${ENGINE_FILES}
        ${GAME_FILES}
)

target_link_libraries(test concurrencpp::concurrencpp ryml::ryml)
target_include_directories(test PRIVATE ${PROJECT_SOURCE_DIR})

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets
)
add_dependencies(test copy_assets)


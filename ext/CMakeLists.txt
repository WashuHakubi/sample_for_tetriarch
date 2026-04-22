set(CATCH_CONFIG_CPP17_STRING_VIEW ON)
add_subdirectory(Catch2)

add_subdirectory(SDL)

add_subdirectory(bgfx.cmake)

set(GLM_BUILD_TESTS OFF)
add_subdirectory(glm)

add_subdirectory(json)

set(BUILD_EXAMPLES OFF)
set(WITH_GFLAGS OFF)
add_subdirectory(ng-log)

add_subdirectory(imgui)
add_subdirectory(stb)

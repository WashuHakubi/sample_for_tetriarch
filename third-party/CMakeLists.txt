add_subdirectory(Catch2)

add_subdirectory(SDL)

add_subdirectory(imgui.cmake)
add_subdirectory(json)

set(LIBCORO_BUILD_EXAMPLES OFF)
set(LIBCORO_FEATURE_NETWORKING OFF)
add_subdirectory(libcoro)

add_subdirectory(spdlog)

add_subdirectory(glm)

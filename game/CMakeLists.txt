file(GLOB_RECURSE game_src_files CONFIGURE_DEPENDS "src/**.cpp")
file(GLOB_RECURSE game_main_files CONFIGURE_DEPENDS "main/**.cpp")

add_library(game_lib STATIC
  ${game_src_files})


target_include_directories(game_lib PUBLIC src)

target_include_directories(game_lib PRIVATE ${stb_lib_dir})

target_link_libraries(game_lib PUBLIC
  glm::glm
  fastgltf::fastgltf
  nlohmann_json::nlohmann_json
  SDL3::SDL3
  simdjson::simdjson
  simdutf::simdutf
  spdlog::spdlog
  wx::wx
)

target_precompile_headers(game_lib PUBLIC
  # standard headers
  <algorithm>
  <cstdint>
  <chrono>
  <filesystem>
  <functional>
  <memory>
  <span>
  <string>
  <string_view>
  <tuple>
  <type_traits>
  <unordered_map>
  <unordered_set>
  <variant>
  <vector>

  # core headers
  <SDL3/SDL.h>
  <spdlog/spdlog.h>
  <wx/reflection/reflection.h>
)

target_compile_definitions(game_lib PUBLIC
  -DGLM_FORCE_DEPTH_ZERO_TO_ONE
  -DGLM_FORCE_LEFT_HANDED
)

target_compile_options(game_lib PRIVATE -Wall -Werror)

add_executable(game ${game_main_files})
target_link_libraries(game PRIVATE game_lib)

add_custom_target(game_build_shaders
  COMMAND sh "${CMAKE_SOURCE_DIR}/build_shaders.sh"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  COMMENT "Building shaders"
  VERBATIM
)

add_dependencies(game game_build_shaders)

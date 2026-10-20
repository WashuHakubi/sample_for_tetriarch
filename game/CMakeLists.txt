file(GLOB_RECURSE game_src CONFIGURE_DEPENDS "src/**.cpp")
file(GLOB_RECURSE main_src CONFIGURE_DEPENDS "main/**.cpp")
file(GLOB_RECURSE tests_src CONFIGURE_DEPENDS "tests/**.cpp")

add_library(game_lib STATIC ${game_src})
target_include_directories(game_lib PUBLIC src)
target_link_libraries(game_lib PUBLIC
  fastgltf::fastgltf
  glm::glm
  lua::lua
  SDL3::SDL3
  simdjson::simdjson
  spdlog::spdlog
  wx::wx
)

target_precompile_headers(game_lib PUBLIC
  <algorithm>
  <cassert>
  <chrono>
  <cstdint>
  <format>
  <functional>
  <memory>
  <string>
  <string_view>
  <type_traits>
  <unordered_map>
  <unordered_set>
  <utility>
  <vector>
  <spdlog/spdlog.h>
  "src/core/types.h"
)

add_executable(game ${main_src})
target_link_libraries(game PRIVATE game_lib)

add_executable(tests ${tests_src})
target_link_libraries(tests PRIVATE
  game_lib
  Catch2::Catch2WithMain
)

catch_discover_tests(tests)

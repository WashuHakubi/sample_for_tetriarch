file(GLOB_RECURSE GAME_SRCS CONFIGURE_DEPENDS "**.cpp")

add_executable(game ${GAME_SRCS})
target_link_libraries(game PRIVATE
  nlohmann_json::nlohmann_json
  SDL3::SDL3
  bgfx::bgfx
  glm::glm
  ng-log::ng-log
)

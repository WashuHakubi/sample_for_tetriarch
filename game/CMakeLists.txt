file(GLOB_RECURSE GAME_SRCS CONFIGURE_DEPENDS "**.cpp")

add_executable(game ${GAME_SRCS})
target_link_libraries(game PRIVATE
  wut_engine
)

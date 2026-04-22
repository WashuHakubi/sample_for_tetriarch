file(GLOB_RECURSE GAME_SRCS CONFIGURE_DEPENDS "**.cpp")

add_executable(game ${GAME_SRCS})
target_link_libraries(game PRIVATE
  wut_engine
)

if(MSVC)
  target_compile_options(game PUBLIC /W4 /WX /wd4100 /wd4996)
else()
  target_compile_options(game PUBLIC
                -Wall
                -Wextra
                -Werror
                -Wno-unused-parameter
                -Wno-unused-function
                -Wno-unused-private-field
                -Wno-missing-field-initializers)
endif()

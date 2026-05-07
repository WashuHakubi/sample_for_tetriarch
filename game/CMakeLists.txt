file(GLOB_RECURSE GAME_SRCS CONFIGURE_DEPENDS "src/**.cpp")

add_executable(game ${GAME_SRCS})
target_link_libraries(game PRIVATE
  SDL3::SDL3
  libcoro
  spdlog
  nlohmann_json
  imgui.cmake
  glm
)

# Add dotnet to game
target_link_directories(game PRIVATE ${DOTNET_HOST_PATH})
target_include_directories(game PRIVATE ${DOTNET_HOST_PATH})
target_link_libraries(game PRIVATE nethost)

# Copy dotnet binaries to the game folder. This is where we'll look for them when running.
add_custom_command(
  TARGET game POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${DOTNET_PATH} $<TARGET_FILE_DIR:game>
  COMMENT "symbolic link resources folder from ${source} => ${destination}"
)

add_custom_command(
  TARGET game POST_BUILD
  COMMAND ${DOTNET_PATH}/dotnet build ${CMAKE_CURRENT_SOURCE_DIR}/csharp/WutGame.csproj
  COMMENT "Build C# game project"
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


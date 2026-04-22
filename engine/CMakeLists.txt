file(GLOB_RECURSE WUT_ENGINE_SRCS CONFIGURE_DEPENDS "src/**.cpp")
add_library(wut_engine STATIC ${WUT_ENGINE_SRCS})
target_include_directories(wut_engine PUBLIC include)
target_link_libraries(wut_engine PUBLIC
  glm::glm
  ng-log::ng-log
  nlohmann_json::nlohmann_json
  SDL3::SDL3
  bgfx
)

if(MSVC)
  target_compile_options(wut_engine PUBLIC /W4 /WX /wd4100 /wd4996)
else()
  target_compile_options(wut_engine PUBLIC
                -Wall
                -Wextra
                -Werror
                -Wno-unused-parameter
                -Wno-unused-function
                -Wno-unused-private-field
                -Wno-missing-field-initializers)
endif()

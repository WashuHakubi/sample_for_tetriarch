file(GLOB_RECURSE WUT_TESTS_SRCS CONFIGURE_DEPENDS "src/**.cpp")
add_executable(wut_tests ${WUT_TESTS_SRCS})
target_link_libraries(wut_tests PRIVATE
  wut_engine
  Catch2::Catch2WithMain)

if(MSVC)
  target_compile_options(wut_tests PUBLIC /W4 /WX /wd4100 /wd4996)
else()
  target_compile_options(wut_tests PUBLIC
                -Wall
                -Wextra
                -Werror
                -Wno-unused-parameter
                -Wno-unused-function
                -Wno-unused-private-field
                -Wno-missing-field-initializers)
endif()

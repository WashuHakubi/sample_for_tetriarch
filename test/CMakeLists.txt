file(GLOB_RECURSE WUT_TESTS_SRCS CONFIGURE_DEPENDS "src/**.cpp")
add_executable(wut_tests ${WUT_TESTS_SRCS})
target_link_libraries(wut_tests PRIVATE
  wut_engine
  Catch2::Catch2WithMain)

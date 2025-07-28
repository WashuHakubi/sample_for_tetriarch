file(GLOB_RECURSE shared_FILES CONFIGURE_DEPENDS "src/**.cpp")

add_library(shared STATIC ${shared_FILES})
target_include_directories(shared PUBLIC include)
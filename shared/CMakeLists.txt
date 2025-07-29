file(GLOB_RECURSE shared_FILES CONFIGURE_DEPENDS "src/**.cpp")

add_library(shared STATIC ${shared_FILES})
target_include_directories(shared PUBLIC include)
target_link_libraries(shared PRIVATE nlohmann_json::nlohmann_json)
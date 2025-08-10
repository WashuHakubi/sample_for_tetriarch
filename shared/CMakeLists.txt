file(GLOB_RECURSE shared_FILES CONFIGURE_DEPENDS "src/**.cpp")

add_library(shared STATIC ${shared_FILES})
target_include_directories(shared PUBLIC include)

target_link_libraries(shared PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(shared PUBLIC
        concurrencpp::concurrencpp
        crossguid
        ng-log::ng-log
        glm::glm-header-only
)

file(GLOB_RECURSE shared_tests_FILES CONFIGURE_DEPENDS "tests/**.cpp")

add_executable(shared_tests ${shared_tests_FILES})
target_link_libraries(shared_tests PRIVATE shared Catch2::Catch2WithMain)

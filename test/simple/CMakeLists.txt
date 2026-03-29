file(GLOB CORE_SOURCES    CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/test/core/*.cpp")
file(GLOB BACKEND_SOURCES CONFIGURE_DEPENDS "*.cpp")
add_executable(logger_tests_simple ${CORE_SOURCES} ${BACKEND_SOURCES})
target_include_directories(logger_tests_simple PRIVATE ${CMAKE_SOURCE_DIR}/src/simple)
target_compile_definitions(logger_tests_simple PRIVATE
    TEST_CONFIG_DIR="${CMAKE_SOURCE_DIR}/test/config")
target_link_libraries(logger_tests_simple PRIVATE logger_simple_auto GTest::gtest_main)
gtest_discover_tests(logger_tests_simple TEST_PREFIX "simple.")

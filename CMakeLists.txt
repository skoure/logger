
cmake_minimum_required(VERSION 3.24)

project(logger)

option(USE_CPPTRACE "Enable stacktrace support via cpptrace" ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch sk::common for HierarchicalNode and other shared utilities
include(FetchContent)
FetchContent_Declare(
    sk_common
    GIT_REPOSITORY https://github.com/skoure/common.git
    GIT_TAG        cdaac9f7448ab6be80e2f9938318839cf884bac4
)
FetchContent_MakeAvailable(sk_common)

include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)

if(EXISTS ${CMAKE_SOURCE_DIR}/build/conan/conan_toolchain.cmake)
    include(${CMAKE_SOURCE_DIR}/build/conan/conan_toolchain.cmake)
endif()

# ---------------------------------------------------------------------------
# cpptrace (optional — used by LoggerBase for exception stacktraces)
# ---------------------------------------------------------------------------
set(CPPTRACE_DEFINES)
set(CPPTRACE_LIBS)
if(USE_CPPTRACE)
    find_package(cpptrace REQUIRED)
    list(APPEND CPPTRACE_DEFINES USE_CPPTRACE)
    list(APPEND CPPTRACE_LIBS cpptrace::cpptrace)
endif()

# ---------------------------------------------------------------------------
# nlohmann_json (header-only, used by JsonConfigParser)
# ---------------------------------------------------------------------------
find_package(nlohmann_json REQUIRED)

# ---------------------------------------------------------------------------
# Core library — backend-independent code
# ---------------------------------------------------------------------------
add_library(logger_core STATIC
    src/LoggerFactory.cpp
    src/LoggerFactoryImpl.cpp
    src/LoggerHierarchy.cpp
    src/LoggerBase.cpp
    src/LoggerUtils.cpp
    src/JsonConfigParser.cpp
    src/LoggerConfigurator.cpp
    src/MarkerFactory.cpp
)
target_compile_definitions(logger_core PUBLIC ${CPPTRACE_DEFINES})
target_link_libraries(logger_core PUBLIC sk::common ${CPPTRACE_LIBS})
target_link_libraries(logger_core PRIVATE nlohmann_json::nlohmann_json)

# ---------------------------------------------------------------------------
# SimpleLogger backend — liblogger_simple
# ---------------------------------------------------------------------------
add_library(logger_simple STATIC
    src/SimpleLogger.cpp
    src/SimpleLoggerBackend.cpp
    src/SimpleLoggerPattern.cpp
)
target_link_libraries(logger_simple PUBLIC logger_core)

add_library(logger_simple_auto INTERFACE)
target_link_libraries(logger_simple_auto INTERFACE
    $<LINK_LIBRARY:WHOLE_ARCHIVE,logger_simple>)

# ---------------------------------------------------------------------------
# spdlog backend — liblogger_spdlog (built if spdlog is available)
# ---------------------------------------------------------------------------
find_package(spdlog QUIET)
if(spdlog_FOUND)
    add_library(logger_spdlog STATIC
        src/SpdlogLogger.cpp
        src/SpdlogBackend.cpp
        src/SpdlogPatternTranslator.cpp
        src/SpdlogThreadLocal.cpp
    )
    target_link_libraries(logger_spdlog PUBLIC logger_core spdlog::spdlog)

    add_library(logger_spdlog_auto INTERFACE)
    target_link_libraries(logger_spdlog_auto INTERFACE
        $<LINK_LIBRARY:WHOLE_ARCHIVE,logger_spdlog>)
else()
    message(STATUS "spdlog not found — logger_spdlog will not be built")
endif()

# ---------------------------------------------------------------------------
# Log4cxx backend — liblogger_log4cxx (built if log4cxx is available)
# ---------------------------------------------------------------------------
find_package(log4cxx QUIET)
if(log4cxx_FOUND)
    add_library(logger_log4cxx STATIC
        src/Log4CxxLogger.cpp
        src/Log4CxxBackend.cpp
        src/Log4CxxPatternTranslator.cpp
    )
    target_include_directories(logger_log4cxx PRIVATE ${log4cxx_INCLUDE_DIRS})
    target_link_libraries(logger_log4cxx PUBLIC logger_core log4cxx)

    add_library(logger_log4cxx_auto INTERFACE)
    target_link_libraries(logger_log4cxx_auto INTERFACE
        $<LINK_LIBRARY:WHOLE_ARCHIVE,logger_log4cxx>)
else()
    message(STATUS "log4cxx not found — logger_log4cxx will not be built")
endif()

# ---------------------------------------------------------------------------
# Install / doc / test targets (top-level builds only)
# ---------------------------------------------------------------------------
if(PROJECT_IS_TOP_LEVEL)

    install(TARGETS logger_core logger_simple logger_simple_auto
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)

    if(TARGET logger_spdlog)
        install(TARGETS logger_spdlog logger_spdlog_auto ARCHIVE DESTINATION lib)
    endif()
    if(TARGET logger_log4cxx)
        install(TARGETS logger_log4cxx logger_log4cxx_auto ARCHIVE DESTINATION lib)
    endif()

    install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/logger DESTINATION include)

    find_package(Doxygen QUIET)
    if(DOXYGEN_FOUND)
        message(STATUS "Doxygen found, enabling documentation target.")
        set(DOXYGEN_IN  ${CMAKE_SOURCE_DIR}/Doxyfile)
        set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/Doxyfile.out)
        add_custom_target(doc
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_IN}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)
    else()
        message(STATUS "Doxygen not found, documentation target will not be available.")
    endif()

    include(GoogleTest)
    find_package(GTest QUIET)

    if(GTest_FOUND)
        enable_testing()

        set(COMMON_TEST_SOURCES
            test/LoggerFactoryTest.cpp
            test/LoggerHierarchyTest.cpp
            test/LoggerUtilsTest.cpp
            test/MarkerFactoryTest.cpp
            test/JsonConfigParserTest.cpp
            test/LoggerConfiguratorTest.cpp
            test/MarkerOverloadTest.cpp
        )

        # SimpleLogger tests
        add_executable(logger_tests_simple
            ${COMMON_TEST_SOURCES}
            test/SimpleLoggerTest.cpp
            test/SimpleLoggerBackendTest.cpp
            test/SimpleLoggerPatternTest.cpp
        )
        target_compile_definitions(logger_tests_simple PRIVATE
            TEST_CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/config")
        target_link_libraries(logger_tests_simple PRIVATE logger_simple_auto GTest::gtest_main)
        gtest_discover_tests(logger_tests_simple TEST_PREFIX "simple.")

        # spdlog tests
        if(TARGET logger_spdlog)
            add_executable(logger_tests_spdlog
                ${COMMON_TEST_SOURCES}
                test/SpdlogLoggerTest.cpp
                test/SpdlogBackendTest.cpp
                test/SpdlogPatternTranslatorTest.cpp
            )
            target_compile_definitions(logger_tests_spdlog PRIVATE
                TEST_CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/config")
            target_link_libraries(logger_tests_spdlog PRIVATE logger_spdlog_auto GTest::gtest_main)
            gtest_discover_tests(logger_tests_spdlog TEST_PREFIX "spdlog.")
        endif()

        # Log4cxx tests
        if(TARGET logger_log4cxx)
            add_executable(logger_tests_log4cxx
                ${COMMON_TEST_SOURCES}
                test/Log4CxxLoggerTest.cpp
                test/Log4CxxBackendTest.cpp
                test/Log4CxxPatternTranslatorTest.cpp
            )
            target_compile_definitions(logger_tests_log4cxx PRIVATE
                TEST_CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/config")
            target_link_libraries(logger_tests_log4cxx PRIVATE logger_log4cxx_auto GTest::gtest_main)
            gtest_discover_tests(logger_tests_log4cxx TEST_PREFIX "log4cxx.")
        endif()

    else()
        message(WARNING "GTest not found: unit tests will not be built")
    endif()

endif() # PROJECT_IS_TOP_LEVEL


cmake_minimum_required(VERSION 3.24)

project(logger)

option(USE_CPPTRACE "Enable stacktrace support via cpptrace" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch sk::common for HierarchicalNode and other shared utilities
include(FetchContent)
FetchContent_Declare(
    sk_common
    GIT_REPOSITORY https://github.com/skoure/common.git
    GIT_TAG        cdaac9f7448ab6be80e2f9938318839cf884bac4
)
FetchContent_MakeAvailable(sk_common)

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
# Backends
# ---------------------------------------------------------------------------
add_subdirectory(src/core)
add_subdirectory(src/simple)
add_subdirectory(src/spdlog)
add_subdirectory(src/log4cxx)

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
        add_subdirectory(test/helpers)
        add_subdirectory(test/simple)
        add_subdirectory(test/spdlog)
        add_subdirectory(test/log4cxx)
    else()
        message(WARNING "GTest not found: unit tests will not be built")
    endif()

endif() # PROJECT_IS_TOP_LEVEL

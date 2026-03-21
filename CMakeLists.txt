cmake_minimum_required(VERSION 3.21)

project(logger)

set(LIBRARY_NAME ${PROJECT_NAME})

# Logger backend options
option(USE_LOG4CXX "Use Log4Cxx" OFF)
option(USE_SPDLOG "Use spdlog" OFF)
option(USE_SIMPLELOGGER "Use SimpleLogger" OFF)
option(USE_CPPTRACE "Enable stacktrace support via cpptrace" ON)

# Enforce mutual exclusion between backends. When switching CMake configurations
# in an IDE, stale cached values from the previous configuration can leave multiple
# backends enabled simultaneously. Force the unused ones off so reconfiguring works
# without requiring a cache clear. Priority: LOG4CXX > SPDLOG > SIMPLELOGGER.
if(USE_LOG4CXX)
    set(USE_SPDLOG OFF CACHE BOOL "Use spdlog" FORCE)
    set(USE_SIMPLELOGGER OFF CACHE BOOL "Use SimpleLogger" FORCE)
elseif(USE_SPDLOG)
    set(USE_LOG4CXX OFF CACHE BOOL "Use Log4Cxx" FORCE)
    set(USE_SIMPLELOGGER OFF CACHE BOOL "Use SimpleLogger" FORCE)
elseif(USE_SIMPLELOGGER)
    set(USE_LOG4CXX OFF CACHE BOOL "Use Log4Cxx" FORCE)
    set(USE_SPDLOG OFF CACHE BOOL "Use spdlog" FORCE)
else()
    message(STATUS "No logger selected, defaulting to SimpleLogger.")
    set(USE_SIMPLELOGGER ON CACHE BOOL "Use SimpleLogger" FORCE)
endif()


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

set(SOURCES
    src/LoggerFactory.cpp
    src/LoggerFactoryImpl.cpp
    src/LoggerHierarchy.cpp
    src/LoggerBase.cpp
    src/LoggerUtils.cpp
)
set(HEADERS
    src/LoggerFactoryImpl.h
    src/LoggerHierarchy.h
    src/LoggerBase.h
    src/LoggerUtils.h
    src/LogRecord.h
)
if(USE_LOG4CXX)
    list(APPEND SOURCES src/Log4CxxLogger.cpp)
    list(APPEND HEADERS src/Log4CxxLogger.h)
elseif(USE_SPDLOG)
    list(APPEND SOURCES src/SpdlogLogger.cpp)
    list(APPEND HEADERS src/SpdlogLogger.h)
else()
    list(APPEND SOURCES src/SimpleLogger.cpp)
    list(APPEND HEADERS src/SimpleLogger.h)
endif()

add_library(logger STATIC ${SOURCES} ${HEADERS})

set(LINK_LIBRARIES)
if(EXISTS ${CMAKE_SOURCE_DIR}/build/conan/conan_toolchain.cmake)
    include(${CMAKE_SOURCE_DIR}/build/conan/conan_toolchain.cmake)
endif()

# Define preprocessor macros for selected logger implementation
set(INCLUDE_DIRS)
set(LOGGER_DEFINES)
if(USE_LOG4CXX)
    find_package(log4cxx REQUIRED)
    message(STATUS "log4cxx include directories: ${log4cxx_INCLUDE_DIRS}")
    list(APPEND INCLUDE_DIRS ${log4cxx_INCLUDE_DIRS})
    list(APPEND LOGGER_DEFINES USE_LOG4CXX)
    list(APPEND LINK_LIBRARIES log4cxx)
elseif(USE_SPDLOG)
    find_package(spdlog REQUIRED)
    list(APPEND LOGGER_DEFINES USE_SPDLOG)
    list(APPEND LINK_LIBRARIES spdlog::spdlog)
elseif(USE_SIMPLELOGGER)
    list(APPEND LOGGER_DEFINES USE_SIMPLELOGGER)
endif()

if(USE_CPPTRACE)
    find_package(cpptrace REQUIRED)
    list(APPEND LOGGER_DEFINES USE_CPPTRACE)
    list(APPEND LINK_LIBRARIES cpptrace::cpptrace)
endif()

target_include_directories(logger PRIVATE ${INCLUDE_DIRS})
target_compile_definitions(logger PRIVATE ${LOGGER_DEFINES})
target_link_libraries(logger PRIVATE ${LINK_LIBRARIES} sk::common)

# Only add install/doc/test targets when this is the top-level project,
# not when consumed via FetchContent by another project.
if(PROJECT_IS_TOP_LEVEL)

    install(TARGETS logger
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)

    install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/logger DESTINATION include)

    find_package(Doxygen QUIET)
    if(DOXYGEN_FOUND)
        message(STATUS "Doxygen found, enabling documentation target.")
        set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/Doxyfile)
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
        set(TEST_SOURCES
            test/LoggerFactoryTest.cpp
            test/LoggerHierarchyTest.cpp
            test/LoggerUtilsTest.cpp
        )
        if(USE_LOG4CXX)
            list(APPEND TEST_SOURCES test/Log4CxxLoggerTest.cpp)
        elseif(USE_SPDLOG)
            list(APPEND TEST_SOURCES test/SpdlogLoggerTest.cpp)
        else()
            list(APPEND TEST_SOURCES test/SimpleLoggerTest.cpp)
        endif()

        add_executable(logger_tests ${TEST_SOURCES})
        target_include_directories(logger_tests PRIVATE ${INCLUDE_DIRS})
        target_compile_definitions(logger_tests PRIVATE ${LOGGER_DEFINES})
        target_link_libraries(logger_tests PRIVATE logger ${LINK_LIBRARIES} sk::common GTest::gtest GTest::gtest_main)
        if(TARGET GTest::gtest)
            target_include_directories(logger_tests PRIVATE ${GTest_INCLUDE_DIRS})
        endif()

        include(GoogleTest)
        gtest_discover_tests(logger_tests)

        enable_testing()
    else()
        message(WARNING "GTest not found: unit tests will not be built")
    endif()

endif() # PROJECT_IS_TOP_LEVEL

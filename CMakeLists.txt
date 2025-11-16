cmake_minimum_required(VERSION 3.21)

project(logger)

set(LIBRARY_NAME ${PROJECT_NAME})

# Logger backend options
option(USE_LOG4CXX "Use Log4Cxx" OFF)
option(USE_SPDLOG "Use spdlog" OFF)
option(USE_SIMPLELOGGER "Use SimpleLogger" OFF)

# Error if more than one backend is enabled
set(logger_count 0)
if(USE_LOG4CXX)
    math(EXPR logger_count "${logger_count} + 1")
endif()
if(USE_SPDLOG)
    math(EXPR logger_count "${logger_count} + 1")
endif()
if(USE_SIMPLELOGGER)
    math(EXPR logger_count "${logger_count} + 1")
endif()

if(logger_count GREATER 1)
    message(FATAL_ERROR "Only one logger can be enabled at a time.")
elseif(logger_count EQUAL 0)
    message(STATUS "No logger selected, defaulting to SimpleLogger.")
    set(USE_SIMPLELOGGER ON CACHE BOOL "Use SimpleLogger" FORCE)
    list(APPEND LOGGER_DEFINES USE_SIMPLELOGGER)
endif()


set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)

file(GLOB SOURCES src/*.cpp src/parsers/*.cpp)
file(GLOB HEADERS src/*.h src/parsers/*.h)

add_library(logger STATIC ${SOURCES} ${HEADERS})

set(LINK_LIBRARIES)
if(EXISTS ${CMAKE_SOURCE_DIR}/build/conan/conan_toolchain.cmake)
    include(${CMAKE_SOURCE_DIR}/build/conan/conan_toolchain.cmake)
endif()

find_package(GTest QUIET)

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

target_include_directories(logger PRIVATE ${INCLUDE_DIRS})
target_compile_definitions(logger PRIVATE ${LOGGER_DEFINES})
target_link_libraries(logger PRIVATE ${LINK_LIBRARIES})

# Install public headers and library
install(TARGETS logger
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

# Install public headers from include/logger
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/logger DESTINATION include)



# GoogleTest integration
include(GoogleTest)

if(GTest_FOUND)
    file(GLOB TEST_SOURCES test/*.cpp)

    add_executable(logger_tests ${TEST_SOURCES})
    target_include_directories(logger_tests PRIVATE ${INCLUDE_DIRS})
    target_compile_definitions(logger_tests PRIVATE ${LOGGER_DEFINES})
    target_link_libraries(logger_tests PRIVATE logger ${LINK_LIBRARIES} GTest::gtest GTest::gtest_main)
    if(TARGET GTest::gtest)
        target_include_directories(logger_tests PRIVATE ${GTest_INCLUDE_DIRS})
    endif()

    include(GoogleTest)
    gtest_discover_tests(logger_tests)

    # Enable testing
    enable_testing()
else()
    message(WARNING "GTest not found: unit tests will not be built")
endif()

find_package(spdlog QUIET)
if(spdlog_FOUND)
    file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")
    add_library(logger_spdlog STATIC ${SOURCES})
    target_include_directories(logger_spdlog PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(logger_spdlog PUBLIC logger_core spdlog::spdlog)

    add_library(logger_spdlog_auto INTERFACE)
    target_link_libraries(logger_spdlog_auto INTERFACE
        $<LINK_LIBRARY:WHOLE_ARCHIVE,logger_spdlog>)
else()
    message(STATUS "spdlog not found — logger_spdlog will not be built")
endif()

find_package(log4cxx QUIET)
if(log4cxx_FOUND)
    file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")
    add_library(logger_log4cxx STATIC ${SOURCES})
    target_include_directories(logger_log4cxx PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${log4cxx_INCLUDE_DIRS}
    )
    target_link_libraries(logger_log4cxx PUBLIC logger_core log4cxx)

    add_library(logger_log4cxx_auto INTERFACE)
    target_link_libraries(logger_log4cxx_auto INTERFACE
        $<LINK_LIBRARY:WHOLE_ARCHIVE,logger_log4cxx>)
else()
    message(STATUS "log4cxx not found — logger_log4cxx will not be built")
endif()

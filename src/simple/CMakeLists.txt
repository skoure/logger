file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")
add_library(logger_simple STATIC ${SOURCES})
target_include_directories(logger_simple PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(logger_simple PUBLIC logger_core)

add_library(logger_simple_auto INTERFACE)
target_link_libraries(logger_simple_auto INTERFACE
    $<LINK_LIBRARY:WHOLE_ARCHIVE,logger_simple>)

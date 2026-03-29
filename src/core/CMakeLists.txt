file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")
add_library(logger_core STATIC ${SOURCES})
target_include_directories(logger_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)
target_compile_definitions(logger_core PUBLIC ${CPPTRACE_DEFINES})
target_link_libraries(logger_core PUBLIC sk::common ${CPPTRACE_LIBS})
target_link_libraries(logger_core PRIVATE nlohmann_json::nlohmann_json)

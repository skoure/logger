add_library(test_helpers STATIC TestUtils.cpp)
target_include_directories(test_helpers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(test_helpers PUBLIC cxx_std_17)

/**
 * @file LoggerUtilsTest.cpp
 * @brief Unit tests for LoggerUtils internal helper functions.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 2026
 */
#include <gtest/gtest.h>
#include <LoggerUtils.h>
#include <stdexcept>

using namespace sk::logger;

TEST(LoggerUtilsTest, IncludesExceptionMessage) {
    std::runtime_error ex("something went wrong");
    std::string result = formatException("operation failed", ex);
    EXPECT_NE(result.find("something went wrong"), std::string::npos);
}

TEST(LoggerUtilsTest, IncludesContextMessage) {
    std::runtime_error ex("disk full");
    std::string result = formatException("write failed", ex);
    EXPECT_NE(result.find("write failed"), std::string::npos);
    EXPECT_NE(result.find("disk full"), std::string::npos);
}

TEST(LoggerUtilsTest, NullMsgOmitsPrefix) {
    std::runtime_error ex("bare error");
    std::string result = formatException(nullptr, ex);
    EXPECT_NE(result.find("bare error"), std::string::npos);
    EXPECT_NE(result[0], ':');
}

TEST(LoggerUtilsTest, IncludesDemangledTypeName) {
    std::runtime_error ex("typed error");
    std::string result = formatException("context", ex);
    EXPECT_NE(result.find("runtime_error"), std::string::npos);
}

TEST(LoggerUtilsTest, ContextMessageSeparatedFromException) {
    std::runtime_error ex("the cause");
    std::string result = formatException("the context", ex);
    // Format should be "the context: <type>: the cause"
    EXPECT_LT(result.find("the context"), result.find("the cause"));
}

#ifdef USE_CPPTRACE
TEST(LoggerUtilsTest, ContainsStacktraceSection) {
    std::runtime_error ex("traced");
    std::string result = formatException("test", ex);
    EXPECT_NE(result.find("Stacktrace:"), std::string::npos);
}

TEST(LoggerUtilsTest, StacktraceContainsFrames) {
    std::runtime_error ex("frames");
    std::string result = formatException("test", ex);
    // cpptrace frames are prefixed with '#'
    EXPECT_NE(result.find('#'), std::string::npos);
}
#endif

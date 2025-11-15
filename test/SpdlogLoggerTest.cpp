/**
 * @file SpdlogLoggerTest.cpp
 * @brief Unit tests for SpdlogLogger formatting and log level filtering.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 15, 2025
 * @date Last modified: November 15, 2025
 */
#include <gtest/gtest.h>

#ifdef USE_SPDLOG
#include <SpdlogLogger.h>
#endif

using namespace sk::logger;

#ifdef USE_SPDLOG

class SpdlogLoggerTest : public ::testing::Test {
protected:
    SpdlogLogger logger;

    SpdlogLoggerTest() : logger("TestLogger") {}

    void SetUp() override {
        logger.setLevel(Logger::Level::Trace);
    }
};

TEST_F(SpdlogLoggerTest, GetterSetterName) {
    EXPECT_EQ(logger.getName(), "TestLogger");
}

TEST_F(SpdlogLoggerTest, GetterSetterLevel) {
    logger.setLevel(Logger::Level::Debug);
    EXPECT_EQ(logger.getLevel(), Logger::Level::Debug);

    logger.setLevel(Logger::Level::Info);
    EXPECT_EQ(logger.getLevel(), Logger::Level::Info);
}

TEST_F(SpdlogLoggerTest, IsFatalEnabled) {
    logger.setLevel(Logger::Level::Fatal);
    EXPECT_TRUE(logger.isFatalEnabled());
}

TEST_F(SpdlogLoggerTest, IsErrorEnabled) {
    logger.setLevel(Logger::Level::Error);
    EXPECT_TRUE(logger.isErrorEnabled());
    EXPECT_FALSE(logger.isWarnEnabled());
}

TEST_F(SpdlogLoggerTest, IsWarnEnabled) {
    logger.setLevel(Logger::Level::Warn);
    EXPECT_TRUE(logger.isWarnEnabled());
    EXPECT_FALSE(logger.isInfoEnabled());
}

TEST_F(SpdlogLoggerTest, IsInfoEnabled) {
    logger.setLevel(Logger::Level::Info);
    EXPECT_TRUE(logger.isInfoEnabled());
    EXPECT_FALSE(logger.isDebugEnabled());
}

TEST_F(SpdlogLoggerTest, IsDebugEnabled) {
    logger.setLevel(Logger::Level::Debug);
    EXPECT_TRUE(logger.isDebugEnabled());
    EXPECT_FALSE(logger.isTraceEnabled());
}

TEST_F(SpdlogLoggerTest, IsTraceEnabled) {
    logger.setLevel(Logger::Level::Trace);
    EXPECT_TRUE(logger.isTraceEnabled());
}

TEST_F(SpdlogLoggerTest, FormatsAndWritesFatal) {
    logger.setLevel(Logger::Level::Fatal);
    EXPECT_NO_THROW(logger.fatal("Fatal error: %d", 42));
}

TEST_F(SpdlogLoggerTest, FormatsAndWritesError) {
    logger.setLevel(Logger::Level::Error);
    EXPECT_NO_THROW(logger.error("Error occurred: %s", "test"));
}

TEST_F(SpdlogLoggerTest, FormatsAndWritesWarn) {
    logger.setLevel(Logger::Level::Warn);
    EXPECT_NO_THROW(logger.warn("Warning at level %d", 5));
}

TEST_F(SpdlogLoggerTest, FormatsAndWritesInfo) {
    logger.setLevel(Logger::Level::Info);
    EXPECT_NO_THROW(logger.info("Info: %s = %d", "value", 123));
}

TEST_F(SpdlogLoggerTest, FormatsAndWritesDebug) {
    logger.setLevel(Logger::Level::Debug);
    EXPECT_NO_THROW(logger.debug("Debug info: %f", 3.14));
}

TEST_F(SpdlogLoggerTest, FormatsAndWritesTrace) {
    logger.setLevel(Logger::Level::Trace);
    EXPECT_NO_THROW(logger.trace("Trace message"));
}

TEST_F(SpdlogLoggerTest, DoesNotLogBelowLevel) {
    logger.setLevel(Logger::Level::Error);
    // Should not throw when logging below current level
    EXPECT_NO_THROW(logger.warn("Should not log"));
    EXPECT_NO_THROW(logger.info("Should not log"));
}

#endif

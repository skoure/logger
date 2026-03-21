/**
 * @file SimpleLoggerTest.cpp
 * @brief Unit tests for SimpleLogger formatting and log level filtering.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 * @date Last modified: November 08, 2025
 */
#include <gtest/gtest.h>
#include <SimpleLogger.h>
#include <sstream>
#include <iostream>
#include <stdexcept>

using namespace sk::logger;

class SimpleLoggerTest : public ::testing::Test {
protected:
    std::stringstream buffer;
    std::streambuf* old;
    SimpleLogger logger;

    SimpleLoggerTest() : logger("TestLogger") {}

    void SetUp() override {
        old = std::clog.rdbuf(buffer.rdbuf());
    }
    void TearDown() override {
        std::clog.rdbuf(old);
    }
};

TEST_F(SimpleLoggerTest, FormatsAndWritesFatal) {
    logger.setLevel(Logger::Level::Fatal);
    logger.fatal("Fatal error: %d", 42);
    std::string output = buffer.str();
    EXPECT_NE(output.find("FATAL [TestLogger] Fatal error: 42"), std::string::npos);
}

TEST_F(SimpleLoggerTest, DoesNotLogBelowLevel) {
    logger.setLevel(Logger::Level::Error);
    logger.warn("Should not appear");
    EXPECT_EQ(buffer.str(), "");
}

TEST_F(SimpleLoggerTest, LogsAtAllLevels) {
    logger.setLevel(Logger::Level::Trace);
    logger.fatal("Fatal");
    logger.error("Error");
    logger.warn("Warn");
    logger.info("Info");
    logger.debug("Debug");
    logger.trace("Trace");
    std::string output = buffer.str();
    EXPECT_NE(output.find("FATAL [TestLogger] Fatal"), std::string::npos);
    EXPECT_NE(output.find("ERROR [TestLogger] Error"), std::string::npos);
    EXPECT_NE(output.find("WARN [TestLogger] Warn"), std::string::npos);
    EXPECT_NE(output.find("INFO [TestLogger] Info"), std::string::npos);
    EXPECT_NE(output.find("DEBUG [TestLogger] Debug"), std::string::npos);
    EXPECT_NE(output.find("TRACE [TestLogger] Trace"), std::string::npos);
}

TEST_F(SimpleLoggerTest, FormattingWorks) {
    logger.setLevel(Logger::Level::Info);
    logger.info("Value: %d, String: %s", 123, "abc");
    std::string output = buffer.str();
    EXPECT_NE(output.find("INFO [TestLogger] Value: 123, String: abc"), std::string::npos);
}

TEST_F(SimpleLoggerTest, ExceptionErrorLogsContextAndMessage) {
    logger.setLevel(Logger::Level::Error);
    try {
        throw std::runtime_error("disk full");
    } catch (const std::exception& ex) {
        logger.error("write failed", ex);
    }
    std::string output = buffer.str();
    EXPECT_NE(output.find("ERROR [TestLogger]"), std::string::npos);
    EXPECT_NE(output.find("write failed"), std::string::npos);
    EXPECT_NE(output.find("disk full"), std::string::npos);
}

TEST_F(SimpleLoggerTest, ExceptionFatalLogsContextAndMessage) {
    logger.setLevel(Logger::Level::Fatal);
    std::runtime_error ex("critical failure");
    logger.fatal("system crash", ex);
    std::string output = buffer.str();
    EXPECT_NE(output.find("FATAL [TestLogger]"), std::string::npos);
    EXPECT_NE(output.find("system crash"), std::string::npos);
    EXPECT_NE(output.find("critical failure"), std::string::npos);
}

TEST_F(SimpleLoggerTest, ExceptionErrorSuppressedWhenBelowLevel) {
    logger.setLevel(Logger::Level::Fatal);
    std::runtime_error ex("suppressed");
    logger.error("should not log", ex);
    EXPECT_EQ(buffer.str(), "");
}

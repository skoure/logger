/**
 * @file SimpleLoggerTest.cpp
 * @brief Unit tests for SimpleLogger formatting and log level filtering.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 */
#include <gtest/gtest.h>
#include <SimpleLogger.h>
#include <logger/MarkerFactory.h>
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

// ---------------------------------------------------------------------------
// Level management API tests
// ---------------------------------------------------------------------------

TEST_F(SimpleLoggerTest, DefaultLevelIsInfo) {
    // Fresh logger (no explicit set) — falls back to Info
    SimpleLogger fresh("FreshLogger");
    EXPECT_EQ(fresh.getLevel(), Logger::Level::Info);
    EXPECT_FALSE(fresh.isLevelExplicitlySet());
}

TEST_F(SimpleLoggerTest, SetLevelMarksExplicit) {
    logger.setLevel(Logger::Level::Debug);
    EXPECT_EQ(logger.getLevel(), Logger::Level::Debug);
    EXPECT_TRUE(logger.isLevelExplicitlySet());
}

TEST_F(SimpleLoggerTest, ClearLevelRevertsToDefault) {
    logger.setLevel(Logger::Level::Warn);
    logger.clearLevel();
    EXPECT_FALSE(logger.isLevelExplicitlySet());
    EXPECT_EQ(logger.getLevel(), Logger::Level::Info);  // root fallback (no parent)
}

TEST_F(SimpleLoggerTest, IsEnabledReflectsLevel) {
    logger.setLevel(Logger::Level::Warn);
    EXPECT_TRUE(logger.isFatalEnabled());
    EXPECT_TRUE(logger.isErrorEnabled());
    EXPECT_TRUE(logger.isWarnEnabled());
    EXPECT_FALSE(logger.isInfoEnabled());
    EXPECT_FALSE(logger.isDebugEnabled());
    EXPECT_FALSE(logger.isTraceEnabled());
}

// ---------------------------------------------------------------------------
// Marker overload tests
// ---------------------------------------------------------------------------

TEST(SimpleLoggerMarkerTest, MarkerNameAppearsInPatternOutput)
{
    auto buf = std::make_shared<std::ostringstream>();
    SimpleLogger logger("SL.Marker");
    logger.setLevel(Logger::Level::Trace);
    logger.setSinks({{ buf, "%M %p %m%n" }});

    auto marker = MarkerFactory::getMarker("SQL");
    logger.info(*marker, "query done");

    EXPECT_NE(buf->str().find("SQL"), std::string::npos);
    EXPECT_NE(buf->str().find("query done"), std::string::npos);
}

TEST(SimpleLoggerMarkerTest, NoMarkerRendersEmptyInPattern)
{
    auto buf = std::make_shared<std::ostringstream>();
    SimpleLogger logger("SL.NoMarker");
    logger.setLevel(Logger::Level::Info);
    logger.setSinks({{ buf, "[%M] %m%n" }});

    logger.info("plain message");

    // %M expands to empty — opening bracket immediately followed by closing
    EXPECT_NE(buf->str().find("[] plain message"), std::string::npos);
}

TEST(SimpleLoggerMarkerTest, MarkerWithExceptionAppearsInOutput)
{
    auto buf = std::make_shared<std::ostringstream>();
    SimpleLogger logger("SL.ExMarker");
    logger.setLevel(Logger::Level::Error);
    logger.setSinks({{ buf, "%M %p %m%n" }});

    auto marker = MarkerFactory::getMarker("DB");
    std::runtime_error ex("connection refused");
    logger.error(*marker, "query failed", ex);

    EXPECT_NE(buf->str().find("DB"), std::string::npos);
    EXPECT_NE(buf->str().find("query failed"), std::string::npos);
}

TEST(SimpleLoggerMarkerTest, MarkerSuppressedBelowLevel)
{
    auto buf = std::make_shared<std::ostringstream>();
    SimpleLogger logger("SL.MarkerSuppressed");
    logger.setLevel(Logger::Level::Fatal);
    logger.setSinks({{ buf, "%M %p %m%n" }});

    auto marker = MarkerFactory::getMarker("NET");
    logger.debug(*marker, "should not appear");

    EXPECT_EQ(buf->str(), "");
}


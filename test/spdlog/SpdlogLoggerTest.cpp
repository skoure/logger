/**
 * @file SpdlogLoggerTest.cpp
 * @brief Unit tests for SpdlogLogger formatting and log level filtering.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 15, 2025
 */
#include <gtest/gtest.h>
#include <SpdlogLogger.h>
#include <SpdlogThreadLocal.h>
#include <logger/MarkerFactory.h>
#include <spdlog/sinks/base_sink.h>
#include <mutex>
#include <stdexcept>

using namespace sk::logger;

// ---------------------------------------------------------------------------
// Sink that captures the marker name and message payload during sink_it_().
// spdlog_tls::markerName is set by SpdlogLogger::append() before the spdlog
// call, so it is still live when sink_it_() fires synchronously.
// ---------------------------------------------------------------------------

class MarkerCapturingSink : public spdlog::sinks::base_sink<std::mutex>
{
public:
    std::string capturedMarker;
    std::string capturedMessage;

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        capturedMarker  = spdlog_tls::markerName ? spdlog_tls::markerName : "";
        capturedMessage = std::string(msg.payload.begin(), msg.payload.end());
    }

    void flush_() override {}
};

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

TEST_F(SpdlogLoggerTest, ExceptionErrorDoesNotThrow) {
    logger.setLevel(Logger::Level::Error);
    std::runtime_error ex("spdlog error");
    EXPECT_NO_THROW(logger.error("context", ex));
}

TEST_F(SpdlogLoggerTest, ExceptionFatalDoesNotThrow) {
    logger.setLevel(Logger::Level::Fatal);
    std::runtime_error ex("spdlog fatal");
    EXPECT_NO_THROW(logger.fatal("context", ex));
}

TEST_F(SpdlogLoggerTest, ExceptionErrorSuppressedWhenBelowLevel) {
    logger.setLevel(Logger::Level::Fatal);
    std::runtime_error ex("should be suppressed");
    EXPECT_NO_THROW(logger.error("suppressed", ex));
}

// ---------------------------------------------------------------------------
// Level management API tests
// ---------------------------------------------------------------------------

TEST_F(SpdlogLoggerTest, DefaultLevelNotExplicitlySet) {
    SpdlogLogger fresh("SpdFreshLogger");
    EXPECT_FALSE(fresh.isLevelExplicitlySet());
}

TEST_F(SpdlogLoggerTest, SetLevelMarksExplicitAndSyncsBackend) {
    logger.setLevel(Logger::Level::Debug);
    EXPECT_TRUE(logger.isLevelExplicitlySet());
    EXPECT_EQ(logger.getLevel(), Logger::Level::Debug);
    EXPECT_TRUE(logger.isDebugEnabled());   // verifies onLevelChanged synced spdlog
}

TEST_F(SpdlogLoggerTest, ClearLevelRevertsToDefault) {
    logger.setLevel(Logger::Level::Warn);
    logger.clearLevel();
    EXPECT_FALSE(logger.isLevelExplicitlySet());
    EXPECT_EQ(logger.getLevel(), Logger::Level::Info);  // root fallback (no parent)
}

// ---------------------------------------------------------------------------
// Marker overload tests
// ---------------------------------------------------------------------------

// Helper: replace the internal spdlog logger's sinks with the given sink.
static std::shared_ptr<MarkerCapturingSink> attachCapturingSink(SpdlogLogger& logger)
{
    auto sink = std::make_shared<MarkerCapturingSink>();
    logger.getInternalLogger()->sinks() = { sink };
    return sink;
}

TEST_F(SpdlogLoggerTest, MarkerNamePassedToSinkOnInfo)
{
    auto sink   = attachCapturingSink(logger);
    auto marker = MarkerFactory::getMarker("SQL");

    logger.info(*marker, "query done");

    EXPECT_EQ(sink->capturedMarker, "SQL");
    EXPECT_NE(sink->capturedMessage.find("query done"), std::string::npos);
}

TEST_F(SpdlogLoggerTest, MarkerNamePassedToSinkOnError)
{
    auto sink   = attachCapturingSink(logger);
    auto marker = MarkerFactory::getMarker("DB");

    logger.error(*marker, "write failed");

    EXPECT_EQ(sink->capturedMarker, "DB");
}

TEST_F(SpdlogLoggerTest, MarkerNamePassedToSinkOnException)
{
    auto sink   = attachCapturingSink(logger);
    auto marker = MarkerFactory::getMarker("NET");
    std::runtime_error ex("timeout");

    logger.error(*marker, "request failed", ex);

    EXPECT_EQ(sink->capturedMarker, "NET");
    EXPECT_NE(sink->capturedMessage.find("request failed"), std::string::npos);
}

TEST_F(SpdlogLoggerTest, NoMarkerLeavesCaptureSinkMarkerEmpty)
{
    auto sink = attachCapturingSink(logger);

    logger.info("plain message");

    EXPECT_EQ(sink->capturedMarker, "");
}

TEST_F(SpdlogLoggerTest, MarkerSuppressedBelowLevel)
{
    auto sink   = attachCapturingSink(logger);
    logger.setLevel(Logger::Level::Fatal);
    auto marker = MarkerFactory::getMarker("Spd.Suppressed");

    logger.debug(*marker, "should not log");
    logger.info (*marker, "should not log");

    // sink_it_() should never have fired
    EXPECT_EQ(sink->capturedMarker, "");
    EXPECT_EQ(sink->capturedMessage, "");
}


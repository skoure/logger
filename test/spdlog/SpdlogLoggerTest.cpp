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
#include <SpdlogBackend.h>
#include <SpdlogLogger.h>
#include <SpdlogThreadLocal.h>
#include <LoggerBase.h>
#include <logger/LevelNames.h>
#include <logger/MarkerFactory.h>
#include <spdlog/sinks/base_sink.h>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>

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

// ---------------------------------------------------------------------------
// Padding tests
//
// Configure a logger with a console sink, disable ANSI colour codes, capture
// stdout, and verify the formatted line contains the expected padded fields.
// ---------------------------------------------------------------------------

namespace {

static std::string captureConsoleLine(const std::string& canonicalPattern,
                                      const std::string& message,
                                      Logger::Level level = Logger::Level::Info,
                                      const Marker* marker = nullptr)
{
    SpdlogBackend backend;
    LoggerPtr loggerPtr = backend.createLogger("Padding.Capture");
    auto* logger = dynamic_cast<SpdlogLogger*>(loggerPtr.get());
    logger->setLevel(Logger::Level::Trace);

    // Use an ostream sink so output goes into a string buffer regardless of
    // platform.  On Windows, stdout_color_sink_mt writes through the Windows
    // Console HANDLE (not the C-runtime FILE), so CaptureStdout() misses it.
    std::ostringstream oss;
    backend.configureLoggerWithOstream(loggerPtr, oss, canonicalPattern);

    if (marker) {
        switch (level) {
        case Logger::Level::Fatal: logger->fatal(*marker, "%s", message.c_str()); break;
        case Logger::Level::Error: logger->error(*marker, "%s", message.c_str()); break;
        case Logger::Level::Warn:  logger->warn (*marker, "%s", message.c_str()); break;
        case Logger::Level::Info:  logger->info (*marker, "%s", message.c_str()); break;
        case Logger::Level::Debug: logger->debug(*marker, "%s", message.c_str()); break;
        case Logger::Level::Trace: logger->trace(*marker, "%s", message.c_str()); break;
        }
    } else {
        switch (level) {
        case Logger::Level::Fatal: logger->fatal("%s", message.c_str()); break;
        case Logger::Level::Error: logger->error("%s", message.c_str()); break;
        case Logger::Level::Warn:  logger->warn ("%s", message.c_str()); break;
        case Logger::Level::Info:  logger->info ("%s", message.c_str()); break;
        case Logger::Level::Debug: logger->debug("%s", message.c_str()); break;
        case Logger::Level::Trace: logger->trace("%s", message.c_str()); break;
        }
    }
    return oss.str();
}

} // namespace

TEST(SpdlogPaddingTest, MarkerLeftAlignedShorterThanWidth)
{
    auto marker = MarkerFactory::getMarker("HI");
    std::string out = captureConsoleLine("[%-10M] %m%n", "msg", Logger::Level::Info, marker.get());
    EXPECT_NE(out.find("[HI        ]"), std::string::npos) << "output: " << out;
}

TEST(SpdlogPaddingTest, MarkerLeftAlignedEmptyIsAllSpaces)
{
    std::string out = captureConsoleLine("[%-10M] %m%n", "msg");
    EXPECT_NE(out.find("[          ]"), std::string::npos) << "output: " << out;
}

TEST(SpdlogPaddingTest, MarkerRightAligned)
{
    auto marker = MarkerFactory::getMarker("HI");
    std::string out = captureConsoleLine("[%10M] %m%n", "msg", Logger::Level::Info, marker.get());
    EXPECT_NE(out.find("[        HI]"), std::string::npos) << "output: " << out;
}

TEST(SpdlogPaddingTest, MarkerExceedsWidthIsNotTruncated)
{
    auto marker = MarkerFactory::getMarker("VERYLONGMARKER");
    std::string out = captureConsoleLine("[%-5M] %m%n", "msg", Logger::Level::Info, marker.get());
    EXPECT_NE(out.find("[VERYLONGMARKER]"), std::string::npos) << "output: " << out;
}

TEST(SpdlogPaddingTest, LevelLeftAligned)
{
    std::string out = captureConsoleLine("[%-5p] %m%n", "msg");
    EXPECT_NE(out.find("[INFO ]"), std::string::npos) << "output: " << out;
}

TEST(SpdlogPaddingTest, FullPatternMatchesExpectedLayout)
{
    auto marker = MarkerFactory::getMarker("GREET");
    std::string out = captureConsoleLine("[%-5p] [%-10M] %m%n", "Hello", Logger::Level::Info, marker.get());
    EXPECT_NE(out.find("[INFO ] [GREET     ] Hello"), std::string::npos)
        << "output: " << out;
}

// ---------------------------------------------------------------------------
// LevelNames customisation tests
// ---------------------------------------------------------------------------

class SpdlogLevelNamesTest : public ::testing::Test {
protected:
    void TearDown() override {
        // Restore factory defaults so other tests are unaffected.
        LoggerBase::setLevelNames(LevelNames{});
    }
};

TEST_F(SpdlogLevelNamesTest, DefaultWarnIsWARN)
{
    std::string out = captureConsoleLine("%p%n", "msg", Logger::Level::Warn);
    EXPECT_NE(out.find("WARN"), std::string::npos) << "output: " << out;
}

TEST_F(SpdlogLevelNamesTest, DefaultInfoIsINFO)
{
    std::string out = captureConsoleLine("%p%n", "msg", Logger::Level::Info);
    EXPECT_NE(out.find("INFO"), std::string::npos) << "output: " << out;
}

TEST_F(SpdlogLevelNamesTest, CustomWarnName)
{
    LevelNames names;
    names.warn = "WARNING";
    LoggerBase::setLevelNames(names);

    std::string out = captureConsoleLine("%p%n", "msg", Logger::Level::Warn);
    EXPECT_NE(out.find("WARNING"), std::string::npos) << "output: " << out;
}

TEST_F(SpdlogLevelNamesTest, CustomWarnNameWithPadding)
{
    LevelNames names;
    names.warn = "WARNING";
    LoggerBase::setLevelNames(names);

    std::string out = captureConsoleLine("[%-8p]%n", "msg", Logger::Level::Warn);
    EXPECT_NE(out.find("[WARNING ]"), std::string::npos) << "output: " << out;
}


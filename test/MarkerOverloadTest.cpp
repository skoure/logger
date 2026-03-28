/**
 * @file MarkerOverloadTest.cpp
 * @brief Unit tests for marker-aware log overloads.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 */
#include <gtest/gtest.h>
#include <logger/Logger.h>
#include <logger/LoggerFactory.h>
#include <logger/MarkerFactory.h>
#include <LogRecord.h>
#include <LoggerBase.h>
#include <SimpleLogger.h>
#include <SimpleLoggerBackend.h>

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace sk::logger;

// ---------------------------------------------------------------------------
// Spy logger: captures the last LogRecord delivered to append()
// ---------------------------------------------------------------------------

class SpyLogger : public LoggerBase
{
public:
    explicit SpyLogger(const std::string& name) : m_name(name) {}

    const std::string getName() override { return m_name; }

    int  appendCallCount = 0;
    LogRecord lastRecord;

protected:
    void append(const LogRecord& record) override
    {
        ++appendCallCount;
        lastRecord = record;
    }

private:
    std::string m_name;
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST(MarkerOverloadTest, InfoWithMarkerSetsRecordMarker)
{
    auto markerPtr = MarkerFactory::getMarker("MO.InfoMarker");
    SpyLogger spy("MarkerOverload.Info");
    spy.setLevel(Logger::Level::Trace);

    spy.info(*markerPtr, "hello %s", "world");

    ASSERT_EQ(spy.appendCallCount, 1);
    ASSERT_NE(spy.lastRecord.marker, nullptr);
    EXPECT_EQ(spy.lastRecord.marker->getName(), "MO.InfoMarker");
    EXPECT_EQ(spy.lastRecord.message, "hello world");
}

TEST(MarkerOverloadTest, ErrorWithMarkerAndException)
{
    auto markerPtr = MarkerFactory::getMarker("MO.ExMarker");
    SpyLogger spy("MarkerOverload.Error");
    spy.setLevel(Logger::Level::Trace);

    std::runtime_error ex("test exception");
    spy.error(*markerPtr, "context", ex);

    ASSERT_EQ(spy.appendCallCount, 1);
    ASSERT_NE(spy.lastRecord.marker, nullptr);
    EXPECT_EQ(spy.lastRecord.marker->getName(), "MO.ExMarker");
    EXPECT_NE(spy.lastRecord.message.find("test exception"), std::string::npos);
}

TEST(MarkerOverloadTest, FatalWithMarkerAndException)
{
    auto markerPtr = MarkerFactory::getMarker("MO.FatalExMarker");
    SpyLogger spy("MarkerOverload.Fatal");
    spy.setLevel(Logger::Level::Trace);

    std::runtime_error ex("fatal test exception");
    spy.fatal(*markerPtr, "fatal context", ex);

    ASSERT_EQ(spy.appendCallCount, 1);
    ASSERT_NE(spy.lastRecord.marker, nullptr);
    EXPECT_EQ(spy.lastRecord.marker->getName(), "MO.FatalExMarker");
}

TEST(MarkerOverloadTest, MarkerOverloadAtDisabledLevelDoesNotCallAppend)
{
    auto markerPtr = MarkerFactory::getMarker("MO.Disabled");
    SpyLogger spy("MarkerOverload.Disabled");
    spy.setLevel(Logger::Level::Fatal);  // only fatal enabled

    spy.debug(*markerPtr, "should not appear");
    spy.trace(*markerPtr, "should not appear");
    spy.info (*markerPtr, "should not appear");

    EXPECT_EQ(spy.appendCallCount, 0);
}

TEST(MarkerOverloadTest, NonMarkerOverloadLeavesMarkerNull)
{
    SpyLogger spy("MarkerOverload.NoMarker");
    spy.setLevel(Logger::Level::Trace);

    spy.info("plain message");

    ASSERT_EQ(spy.appendCallCount, 1);
    EXPECT_EQ(spy.lastRecord.marker, nullptr);
}

TEST(MarkerOverloadTest, AllMarkerLevels)
{
    auto markerPtr = MarkerFactory::getMarker("MO.AllLevels");
    SpyLogger spy("MarkerOverload.AllLevels");
    spy.setLevel(Logger::Level::Trace);

    spy.fatal(*markerPtr, "fatal");
    spy.error(*markerPtr, "error");
    spy.warn (*markerPtr, "warn");
    spy.info (*markerPtr, "info");
    spy.debug(*markerPtr, "debug");
    spy.trace(*markerPtr, "trace");

    EXPECT_EQ(spy.appendCallCount, 6);
}

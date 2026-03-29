/**
 * @file SimpleLoggerPatternTest.cpp
 * @brief Unit tests for SimpleLoggerPattern::render().
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 */
#include <gtest/gtest.h>
#include <SimpleLoggerPattern.h>
#include <LogRecord.h>
#include <logger/MarkerFactory.h>
#include <chrono>
#include <string>
#include <thread>

using namespace sk::logger;

// ---------------------------------------------------------------------------
// Helper: build a minimal LogRecord
// ---------------------------------------------------------------------------

static LogRecord makeRecord(Logger::Level level = Logger::Level::Info,
                             const std::string& logger = "TestLogger",
                             const std::string& message = "test message",
                             const Marker* marker = nullptr)
{
    LogRecord r;
    r.level      = level;
    r.loggerName = logger;
    r.message    = message;
    r.timestamp  = std::chrono::system_clock::now();
    r.threadId   = std::this_thread::get_id();
    r.threadName = "main";
    r.marker     = marker;
    return r;
}

// ---------------------------------------------------------------------------
// Token expansion tests
// ---------------------------------------------------------------------------

TEST(SimpleLoggerPatternTest, MessageToken)
{
    LogRecord r = makeRecord(Logger::Level::Info, "L", "hello world");
    EXPECT_EQ(SimpleLoggerPattern::render("%m", r), "hello world");
}

TEST(SimpleLoggerPatternTest, LevelToken)
{
    LogRecord r = makeRecord(Logger::Level::Debug);
    EXPECT_EQ(SimpleLoggerPattern::render("%p", r), "DEBUG");
}

TEST(SimpleLoggerPatternTest, LoggerNameToken)
{
    LogRecord r = makeRecord(Logger::Level::Info, "App.Database");
    EXPECT_EQ(SimpleLoggerPattern::render("%c", r), "App.Database");
}

TEST(SimpleLoggerPatternTest, ThreadIdToken)
{
    LogRecord r = makeRecord();
    std::string result = SimpleLoggerPattern::render("%t", r);
    // Should be non-empty
    EXPECT_FALSE(result.empty());
}

TEST(SimpleLoggerPatternTest, ThreadNameToken)
{
    LogRecord r = makeRecord();
    r.threadName = "mythread";
    EXPECT_EQ(SimpleLoggerPattern::render("%T", r), "mythread");
}

TEST(SimpleLoggerPatternTest, NewlineToken)
{
    LogRecord r = makeRecord();
    EXPECT_EQ(SimpleLoggerPattern::render("%n", r), "\n");
}

TEST(SimpleLoggerPatternTest, MarkerTokenWithNullMarker)
{
    LogRecord r = makeRecord(Logger::Level::Info, "L", "msg", nullptr);
    EXPECT_EQ(SimpleLoggerPattern::render("%M", r), "");
}

TEST(SimpleLoggerPatternTest, MarkerTokenWithMarker)
{
    auto markerPtr = MarkerFactory::getMarker("SLP.TestMarker");
    LogRecord r = makeRecord(Logger::Level::Info, "L", "msg", markerPtr.get());
    EXPECT_EQ(SimpleLoggerPattern::render("%M", r), "SLP.TestMarker");
}

TEST(SimpleLoggerPatternTest, DateTokenFormatsTimestamp)
{
    LogRecord r = makeRecord();
    std::string result = SimpleLoggerPattern::render("%d{%Y}", r);
    // Should be a 4-digit year
    EXPECT_EQ(result.size(), 4u);
    EXPECT_NE(result.find("20"), std::string::npos);
}

TEST(SimpleLoggerPatternTest, UnknownTokenPassesThrough)
{
    LogRecord r = makeRecord();
    EXPECT_EQ(SimpleLoggerPattern::render("%q", r), "%q");
}

TEST(SimpleLoggerPatternTest, FullPattern)
{
    LogRecord r = makeRecord(Logger::Level::Warn, "MyApp", "something happened");
    r.threadName = "";
    std::string result = SimpleLoggerPattern::render("[%p] [%c] %m%n", r);
    EXPECT_EQ(result, "[WARN] [MyApp] something happened\n");
}

TEST(SimpleLoggerPatternTest, LiteralPercentEscaping)
{
    LogRecord r = makeRecord();
    EXPECT_EQ(SimpleLoggerPattern::render("%%", r), "%");
}

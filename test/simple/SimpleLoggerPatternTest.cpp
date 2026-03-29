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

// ---------------------------------------------------------------------------
// Modifier tests
// ---------------------------------------------------------------------------

TEST(SimpleLoggerPatternTest, MinWidthPadsLeft)
{
    // Right-align (default): pad on left
    LogRecord r = makeRecord(Logger::Level::Info, "L", "foo");
    EXPECT_EQ(SimpleLoggerPattern::render("%10m", r), "       foo");
}

TEST(SimpleLoggerPatternTest, MinWidthLeftAlign)
{
    // Left-align: pad on right
    LogRecord r = makeRecord(Logger::Level::Info, "L", "foo");
    EXPECT_EQ(SimpleLoggerPattern::render("%-10m", r), "foo       ");
}

TEST(SimpleLoggerPatternTest, MinWidthExact_NoPad)
{
    LogRecord r = makeRecord(Logger::Level::Info, "L", "foo");
    EXPECT_EQ(SimpleLoggerPattern::render("%3m", r), "foo");
}

TEST(SimpleLoggerPatternTest, MinWidthLonger_NoTrunc)
{
    // min-width never truncates
    LogRecord r = makeRecord(Logger::Level::Info, "L", "0123456789");
    EXPECT_EQ(SimpleLoggerPattern::render("%3m", r), "0123456789");
}

TEST(SimpleLoggerPatternTest, MaxWidthTruncatesLeft)
{
    // Truncates from the left (log4j default)
    LogRecord r = makeRecord(Logger::Level::Info, "L", "hello world");
    EXPECT_EQ(SimpleLoggerPattern::render("%.5m", r), "world");
}

TEST(SimpleLoggerPatternTest, MaxWidthExact_NoChange)
{
    LogRecord r = makeRecord(Logger::Level::Info, "L", "hello");
    EXPECT_EQ(SimpleLoggerPattern::render("%.5m", r), "hello");
}

TEST(SimpleLoggerPatternTest, MaxWidthShorter_NoChange)
{
    LogRecord r = makeRecord(Logger::Level::Info, "L", "hi");
    EXPECT_EQ(SimpleLoggerPattern::render("%.5m", r), "hi");
}

TEST(SimpleLoggerPatternTest, MaxWidthZero_EmptyOutput)
{
    LogRecord r = makeRecord(Logger::Level::Info, "L", "hello");
    EXPECT_EQ(SimpleLoggerPattern::render("%.0m", r), "");
}

TEST(SimpleLoggerPatternTest, MinAndMaxWidth_TruncateThenPad)
{
    // Truncate "hello world" to 5 chars → "world", then pad to 10 (left-align)
    LogRecord r = makeRecord(Logger::Level::Info, "L", "hello world");
    EXPECT_EQ(SimpleLoggerPattern::render("%-10.5m", r), "world     ");
}

TEST(SimpleLoggerPatternTest, MinAndMaxWidth_PadOnly)
{
    // "hi" is shorter than max (5) so no truncation, then pad to 10 (right-align)
    LogRecord r = makeRecord(Logger::Level::Info, "L", "hi");
    EXPECT_EQ(SimpleLoggerPattern::render("%10.5m", r), "        hi");
}

TEST(SimpleLoggerPatternTest, ModifierOnLevel)
{
    LogRecord r = makeRecord(Logger::Level::Info);
    EXPECT_EQ(SimpleLoggerPattern::render("%-10p", r), "INFO      ");
}

TEST(SimpleLoggerPatternTest, ModifierOnLoggerName)
{
    LogRecord r = makeRecord(Logger::Level::Info, "App");
    EXPECT_EQ(SimpleLoggerPattern::render("%20c", r), "                 App");
}

TEST(SimpleLoggerPatternTest, ModifierOnDate)
{
    LogRecord r = makeRecord();
    std::string result = SimpleLoggerPattern::render("%10d{%Y}", r);
    // 4-digit year right-padded to 10 chars
    EXPECT_EQ(result.size(), 10u);
    EXPECT_EQ(result.substr(0, 6), "      "); // 6 leading spaces
    EXPECT_NE(result.find("20"), std::string::npos);
}

TEST(SimpleLoggerPatternTest, ModifierOnMarkerEmpty)
{
    LogRecord r = makeRecord(Logger::Level::Info, "L", "msg", nullptr);
    EXPECT_EQ(SimpleLoggerPattern::render("%10M", r), "          ");
}

TEST(SimpleLoggerPatternTest, ModifierOnMarkerPresent)
{
    auto markerPtr = MarkerFactory::getMarker("DB");
    LogRecord r = makeRecord(Logger::Level::Info, "L", "msg", markerPtr.get());
    EXPECT_EQ(SimpleLoggerPattern::render("%-10M", r), "DB        ");
}

TEST(SimpleLoggerPatternTest, ModifierOnNewlineIgnored)
{
    LogRecord r = makeRecord();
    EXPECT_EQ(SimpleLoggerPattern::render("%-5n", r), "\n");
}

TEST(SimpleLoggerPatternTest, DoublePercent_Unaffected)
{
    LogRecord r = makeRecord();
    EXPECT_EQ(SimpleLoggerPattern::render("%%", r), "%");
}

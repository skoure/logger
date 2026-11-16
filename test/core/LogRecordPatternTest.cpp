/**
 * @file LogRecordPatternTest.cpp
 * @brief Unit tests for compiled log-record pattern rendering.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 */
#include <gtest/gtest.h>
#include <LogRecordPattern.h>
#include <LoggerBase.h>
#include <LoggerUtils.h>
#include <LogRecord.h>
#include <logger/LevelNames.h>
#include <logger/Exceptions.h>
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
                             const std::vector<const Marker*>& markers = {})
{
    LogRecord r;
    r.level      = level;
    r.loggerName = logger;
    r.message    = message;
    r.timestamp  = std::chrono::system_clock::now();
    r.threadId   = std::this_thread::get_id();
    r.threadName = "main";
    r.markers    = markers;
    return r;
}

// ---------------------------------------------------------------------------
// Token expansion tests
// ---------------------------------------------------------------------------

TEST(LogRecordPatternTest, MessageToken)
{
    LogRecord r = makeRecord(Logger::Level::Info, "L", "hello world");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%m"), r), "hello world");
}

TEST(LogRecordPatternTest, LevelToken)
{
    LogRecord r = makeRecord(Logger::Level::Debug);
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%p"), r), "DEBUG");
}

TEST(LogRecordPatternTest, LoggerNameToken)
{
    LogRecord r = makeRecord(Logger::Level::Info, "App.Database");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%c"), r), "App.Database");
}

TEST(LogRecordPatternTest, ThreadIdToken)
{
    LogRecord r = makeRecord();
    std::string result = LogRecordPattern::render(CompiledPattern("%t"), r);
    // Should be non-empty
    EXPECT_FALSE(result.empty());
}

TEST(LogRecordPatternTest, ThreadNameToken)
{
    LogRecord r = makeRecord();
    r.threadName = "mythread";
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%T"), r), "mythread");
}

TEST(LogRecordPatternTest, NewlineToken)
{
    LogRecord r = makeRecord();
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%n"), r), eol);
}

TEST(LogRecordPatternTest, MarkerTokenWithNullMarker)
{
    LogRecord r = makeRecord(Logger::Level::Info, "L", "msg");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%M"), r), "");
}

TEST(LogRecordPatternTest, MarkerTokenWithMarker)
{
    auto markerPtr = MarkerFactory::getMarker("SLP.TestMarker");
    LogRecord r = makeRecord(Logger::Level::Info, "L", "msg", {markerPtr.get()});
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%M"), r), "SLP.TestMarker");
}

TEST(LogRecordPatternTest, BareMarkerTokenRetainsCommaSpaceSeparator)
{
    auto first = MarkerFactory::getMarker("FIRST");
    auto second = MarkerFactory::getMarker("SECOND");
    LogRecord r = makeRecord(Logger::Level::Info, "L", "msg",
                             {first.get(), second.get()});

    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%M"), r), "FIRST, SECOND");
}

TEST(LogRecordPatternTest, MarkerIncludeModeUsesEventOrder)
{
    auto sql = MarkerFactory::getMarker("SQL");
    auto security = MarkerFactory::getMarker("SECURITY");
    auto op = MarkerFactory::getMarker("OP-LOG");
    LogRecord r = makeRecord(Logger::Level::Info, "L", "msg",
                             {sql.get(), security.get(), op.get()});

    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%M{+:OP-LOG,SECURITY;sep=|}"), r),
              "SECURITY|OP-LOG");
}

TEST(LogRecordPatternTest, CompiledPatternCachesEachTopLevelMarkerToken)
{
    auto first = MarkerFactory::getMarker("FIRST");
    auto second = MarkerFactory::getMarker("SECOND");
    LogRecord record = makeRecord(Logger::Level::Info, "L", "message",
                                  {first.get(), second.get()});
    const CompiledPattern pattern(
        "%M{+:SECOND;sep=|} %%M %d{%%M} %M{;sep=/}");

    EXPECT_EQ(LogRecordPattern::render(pattern, record),
              "SECOND %M %M FIRST/SECOND");
    EXPECT_EQ(LogRecordPattern::render(pattern, record),
              "SECOND %M %M FIRST/SECOND");
}

TEST(LogRecordPatternTest, CompiledPatternCoalescesLiteralRunsAndCachesPayloads)
{
    const CompiledPattern pattern("before %% after %q %10m%%%-5.3d{%Y}%n");
    const auto& tokens = pattern.tokens();

    ASSERT_EQ(tokens.size(), 5u);
    ASSERT_TRUE(std::holds_alternative<CompiledPattern::LiteralToken>(tokens[0]));
    EXPECT_EQ(std::get<CompiledPattern::LiteralToken>(tokens[0]).text,
              "before % after %q ");
    ASSERT_TRUE(std::holds_alternative<CompiledPattern::MessageToken>(tokens[1]));
    EXPECT_EQ(std::get<CompiledPattern::MessageToken>(tokens[1]).modifier.minWidth,
              10);
    ASSERT_TRUE(std::holds_alternative<CompiledPattern::LiteralToken>(tokens[2]));
    EXPECT_EQ(std::get<CompiledPattern::LiteralToken>(tokens[2]).text, "%");
    ASSERT_TRUE(std::holds_alternative<CompiledPattern::DateToken>(tokens[3]));
    const auto& date = std::get<CompiledPattern::DateToken>(tokens[3]);
    EXPECT_EQ(date.format, "%Y");
    EXPECT_TRUE(date.modifier.leftAlign);
    EXPECT_EQ(date.modifier.minWidth, 5);
    EXPECT_EQ(date.modifier.maxWidth, 3);
    EXPECT_TRUE(std::holds_alternative<CompiledPattern::NewlineToken>(tokens[4]));
}

TEST(LogRecordPatternTest, CompiledPatternPreservesPercentAndIncompleteModifierBehavior)
{
    LogRecord r = makeRecord();

    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("x%%y %10q z"), r),
              "x%y %q z");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("before%"), r),
              "before%");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%-"), r), "%");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%5"), r), "%");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%-5%"), r), "%");
}

TEST(LogRecordPatternTest, CompiledPatternDateAndEscapedMarkerTextStayLiteral)
{
    auto marker = MarkerFactory::getMarker("ACTIVE");
    LogRecord r = makeRecord(Logger::Level::Info, "L", "msg", {marker.get()});
    const CompiledPattern pattern("%%M|%d{%%M}|%M");

    ASSERT_EQ(pattern.tokens().size(), 4u);
    EXPECT_TRUE(std::holds_alternative<CompiledPattern::LiteralToken>(pattern.tokens()[0]));
    EXPECT_TRUE(std::holds_alternative<CompiledPattern::DateToken>(pattern.tokens()[1]));
    EXPECT_TRUE(std::holds_alternative<CompiledPattern::MarkerToken>(pattern.tokens()[3]));
    EXPECT_EQ(LogRecordPattern::render(pattern, r), "%M|%M|ACTIVE");
}

TEST(LogRecordPatternTest, UnclosedDateFallbackContinuesParsingFollowingTokens)
{
    LogRecord r = makeRecord(Logger::Level::Info, "L", "message");

    EXPECT_EQ(LogRecordPattern::render(
                  CompiledPattern("%-8d{prefix %m tail"), r),
              "%d{     {prefix message tail");
}

TEST(LogRecordPatternTest, CompiledPatternRejectsMalformedMarkerOptions)
{
    EXPECT_THROW(CompiledPattern("%M{+:MARKER;unknown=x}"),
                 PatternLayoutParseException);
    EXPECT_THROW(CompiledPattern("%M{"),
                 PatternLayoutParseException);
}

TEST(LogRecordPatternTest, MarkerBareListDefaultsToInclude)
{
    auto sql = MarkerFactory::getMarker("SQL");
    auto security = MarkerFactory::getMarker("SECURITY");
    LogRecord r = makeRecord(Logger::Level::Info, "L", "msg",
                             {sql.get(), security.get()});

    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%M{SECURITY}"), r), "SECURITY");
}

TEST(LogRecordPatternTest, MarkerExcludeModeAndAllMode)
{
    auto activity = MarkerFactory::getMarker("ACTIVITY-LOG");
    auto security = MarkerFactory::getMarker("SECURITY");
    LogRecord r = makeRecord(Logger::Level::Info, "L", "msg",
                             {activity.get(), security.get()});

    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%M{-:ACTIVITY-LOG;sep=|}"), r), "SECURITY");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%M{;sep=|}"), r), "ACTIVITY-LOG|SECURITY");
}

TEST(LogRecordPatternTest, MarkerSeparatorPreservesUnquotedSpacesAndStripsQuotes)
{
    auto first = MarkerFactory::getMarker("FIRST");
    auto second = MarkerFactory::getMarker("SECOND");
    LogRecord r = makeRecord(Logger::Level::Info, "L", "msg",
                             {first.get(), second.get()});

    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%M"), r), "FIRST, SECOND");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%M{;sep= | }"), r), "FIRST | SECOND");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%M{;sep=' | '}"), r), "FIRST | SECOND");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%M{;sep=it's}"), r), "FIRSTit'sSECOND");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%M{;sep=\";|\"}"), r), "FIRST;|SECOND");
}

TEST(LogRecordPatternTest, MarkerOptionsWithConflictingModesAreRejected)
{
    EXPECT_THROW(CompiledPattern("%M{+:MARKER,-:OTHER}"),
                 PatternLayoutParseException);
}

TEST(LogRecordPatternTest, InvalidMarkerOptionsAreRejectedDuringCompilation)
{
    EXPECT_THROW(CompiledPattern("before%M{+:MARKER;unknown=x}after %m"),
                 PatternLayoutParseException);
    EXPECT_THROW(CompiledPattern("before%M{"), PatternLayoutParseException);
    EXPECT_THROW(CompiledPattern("[%-20M{;unknown=x}]"),
                 PatternLayoutParseException);
}

TEST(LogRecordPatternTest, DateTokenFormatsTimestamp)
{
    LogRecord r = makeRecord();
    std::string result = LogRecordPattern::render(CompiledPattern("%d{%Y}"), r);
    // Should be a 4-digit year
    EXPECT_EQ(result.size(), 4u);
    EXPECT_NE(result.find("20"), std::string::npos);
}

TEST(LogRecordPatternTest, UnknownTokenPassesThrough)
{
    LogRecord r = makeRecord();
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%q"), r), "%q");
}

TEST(LogRecordPatternTest, FullPattern)
{
    LogRecord r = makeRecord(Logger::Level::Warn, "MyApp", "something happened");
    r.threadName = "";
    std::string result = LogRecordPattern::render(CompiledPattern("[%p] [%c] %m%n"), r);
    std::string expected = std::string("[WARN] [MyApp] something happened") + eol;
    EXPECT_EQ(result, expected);
}

TEST(LogRecordPatternTest, LiteralPercentEscaping)
{
    LogRecord r = makeRecord();
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%%"), r), "%");
}

// ---------------------------------------------------------------------------
// Modifier tests
// ---------------------------------------------------------------------------

TEST(LogRecordPatternTest, MinWidthPadsLeft)
{
    // Right-align (default): pad on left
    LogRecord r = makeRecord(Logger::Level::Info, "L", "foo");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%10m"), r), "       foo");
}

TEST(LogRecordPatternTest, MinWidthLeftAlign)
{
    // Left-align: pad on right
    LogRecord r = makeRecord(Logger::Level::Info, "L", "foo");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%-10m"), r), "foo       ");
}

TEST(LogRecordPatternTest, MinWidthExact_NoPad)
{
    LogRecord r = makeRecord(Logger::Level::Info, "L", "foo");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%3m"), r), "foo");
}

TEST(LogRecordPatternTest, MinWidthLonger_NoTrunc)
{
    // min-width never truncates
    LogRecord r = makeRecord(Logger::Level::Info, "L", "0123456789");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%3m"), r), "0123456789");
}

TEST(LogRecordPatternTest, MaxWidthTruncatesLeft)
{
    // Truncates from the left (log4j default)
    LogRecord r = makeRecord(Logger::Level::Info, "L", "hello world");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%.5m"), r), "world");
}

TEST(LogRecordPatternTest, MaxWidthExact_NoChange)
{
    LogRecord r = makeRecord(Logger::Level::Info, "L", "hello");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%.5m"), r), "hello");
}

TEST(LogRecordPatternTest, MaxWidthShorter_NoChange)
{
    LogRecord r = makeRecord(Logger::Level::Info, "L", "hi");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%.5m"), r), "hi");
}

TEST(LogRecordPatternTest, MaxWidthZero_EmptyOutput)
{
    LogRecord r = makeRecord(Logger::Level::Info, "L", "hello");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%.0m"), r), "");
}

TEST(LogRecordPatternTest, MinAndMaxWidth_TruncateThenPad)
{
    // Truncate "hello world" to 5 chars → "world", then pad to 10 (left-align)
    LogRecord r = makeRecord(Logger::Level::Info, "L", "hello world");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%-10.5m"), r), "world     ");
}

TEST(LogRecordPatternTest, MinAndMaxWidth_PadOnly)
{
    // "hi" is shorter than max (5) so no truncation, then pad to 10 (right-align)
    LogRecord r = makeRecord(Logger::Level::Info, "L", "hi");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%10.5m"), r), "        hi");
}

TEST(LogRecordPatternTest, ModifierOnLevel)
{
    LogRecord r = makeRecord(Logger::Level::Info);
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%-10p"), r), "INFO      ");
}

TEST(LogRecordPatternTest, ModifierOnLoggerName)
{
    LogRecord r = makeRecord(Logger::Level::Info, "App");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%20c"), r), "                 App");
}

TEST(LogRecordPatternTest, ModifierOnDate)
{
    LogRecord r = makeRecord();
    std::string result = LogRecordPattern::render(CompiledPattern("%10d{%Y}"), r);
    // 4-digit year right-padded to 10 chars
    EXPECT_EQ(result.size(), 10u);
    EXPECT_EQ(result.substr(0, 6), "      "); // 6 leading spaces
    EXPECT_NE(result.find("20"), std::string::npos);
}

TEST(LogRecordPatternTest, ModifierOnMarkerEmpty)
{
    LogRecord r = makeRecord(Logger::Level::Info, "L", "msg");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%10M"), r), "          ");
}

TEST(LogRecordPatternTest, ModifierOnMarkerPresent)
{
    auto markerPtr = MarkerFactory::getMarker("DB");
    LogRecord r = makeRecord(Logger::Level::Info, "L", "msg", {markerPtr.get()});
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%-10M"), r), "DB        ");
}

TEST(LogRecordPatternTest, ModifierOnNewlineIgnored)
{
    LogRecord r = makeRecord();
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%-5n"), r), eol);
}

TEST(LogRecordPatternTest, DoublePercent_Unaffected)
{
    LogRecord r = makeRecord();
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%%"), r), "%");
}

// ---------------------------------------------------------------------------
// LevelNames customisation tests
// ---------------------------------------------------------------------------

class LevelNamesTest : public ::testing::Test {
protected:
    void TearDown() override {
        // Restore factory defaults so other tests are unaffected.
        LoggerBase::setLevelNames(LevelNames{});
    }
};

TEST_F(LevelNamesTest, DefaultWarnIsWARN)
{
    LogRecord r = makeRecord(Logger::Level::Warn);
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%p"), r), "WARN");
}

TEST_F(LevelNamesTest, DefaultInfoIsINFO)
{
    LogRecord r = makeRecord(Logger::Level::Info);
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%p"), r), "INFO");
}

TEST_F(LevelNamesTest, CustomWarnName)
{
    LevelNames names;
    names.warn = "WARNING";
    LoggerBase::setLevelNames(names);

    LogRecord r = makeRecord(Logger::Level::Warn);
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%p"), r), "WARNING");
}

TEST_F(LevelNamesTest, AllLevelsCustom)
{
    LevelNames names;
    names.fatal = "CRIT";
    names.error = "ERR";
    names.warn  = "WRN";
    names.info  = "INF";
    names.debug = "DBG";
    names.trace = "TRC";
    LoggerBase::setLevelNames(names);

    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%p"), makeRecord(Logger::Level::Fatal)), "CRIT");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%p"), makeRecord(Logger::Level::Error)), "ERR");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%p"), makeRecord(Logger::Level::Warn)),  "WRN");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%p"), makeRecord(Logger::Level::Info)),  "INF");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%p"), makeRecord(Logger::Level::Debug)), "DBG");
    EXPECT_EQ(LogRecordPattern::render(CompiledPattern("%p"), makeRecord(Logger::Level::Trace)), "TRC");
}

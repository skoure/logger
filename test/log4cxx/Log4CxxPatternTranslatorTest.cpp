/**
 * @file Log4CxxPatternTranslatorTest.cpp
 * @brief Unit tests for Log4CxxPatternTranslator.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 */
#include <gtest/gtest.h>
#include <Log4CxxPatternTranslator.h>

using namespace sk::logger;

TEST(Log4CxxPatternTranslatorTest, MessageTokenPassesThrough)
{
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%m"), "%m");
}

TEST(Log4CxxPatternTranslatorTest, LevelTokenPassesThrough)
{
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%p"), "%X{level}");
}

TEST(Log4CxxPatternTranslatorTest, LoggerNamePassesThrough)
{
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%c"), "%c");
}

TEST(Log4CxxPatternTranslatorTest, ThreadIdPassesThrough)
{
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%t"), "%t");
}

TEST(Log4CxxPatternTranslatorTest, NewlinePassesThrough)
{
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%n"), "%n");
}

TEST(Log4CxxPatternTranslatorTest, MarkerTokenTranslated)
{
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%M"), "%X{marker}");
}

TEST(Log4CxxPatternTranslatorTest, ThreadNameTokenTranslated)
{
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%T"), "%t");
}

TEST(Log4CxxPatternTranslatorTest, DateTokenStrftimeToJava)
{
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%d{%Y-%m-%d %H:%M:%S}"),
              "%d{yyyy-MM-dd HH:mm:ss}");
}

TEST(Log4CxxPatternTranslatorTest, MixedPattern)
{
    std::string result = Log4CxxPatternTranslator::translate(
        "[%d{%Y-%m-%d %H:%M:%S}] [%p] %m%n");
    EXPECT_EQ(result, "[%d{yyyy-MM-dd HH:mm:ss}] [%X{level}] %m%n");
}

TEST(Log4CxxPatternTranslatorTest, MarkerInFullPattern)
{
    std::string result = Log4CxxPatternTranslator::translate(
        "[%p] [%M] %m%n");
    EXPECT_EQ(result, "[%X{level}] [%X{marker}] %m%n");
}

// ---------------------------------------------------------------------------
// Modifier tests
// ---------------------------------------------------------------------------

TEST(Log4CxxPatternTranslatorTest, MinWidthRightAlign)
{
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%10m"), "%10m");
}

TEST(Log4CxxPatternTranslatorTest, MinWidthLeftAlign)
{
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%-10p"), "%-10X{level}");
}

TEST(Log4CxxPatternTranslatorTest, MaxWidthOnly)
{
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%.5c"), "%.5c");
}

TEST(Log4CxxPatternTranslatorTest, MinAndMaxWidth)
{
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%-20.30m"), "%-20.30m");
}

TEST(Log4CxxPatternTranslatorTest, ModifierOnThreadName)
{
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%-10T"), "%-10t");
}

TEST(Log4CxxPatternTranslatorTest, ModifierOnMarker)
{
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%-10M"), "%-10X{marker}");
}

TEST(Log4CxxPatternTranslatorTest, ModifierOnDateWithFmt)
{
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%-25d{%Y-%m-%d %H:%M:%S}"),
              "%-25d{yyyy-MM-dd HH:mm:ss}");
}

TEST(Log4CxxPatternTranslatorTest, ModifierOnBareDate)
{
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%10d"), "%10d");
}

TEST(Log4CxxPatternTranslatorTest, ModifierOnUnknownToken)
{
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%-5q"), "%-5q");
}

TEST(Log4CxxPatternTranslatorTest, ModifierInFullPattern)
{
    std::string result = Log4CxxPatternTranslator::translate("[%-5p] %-20c: %m%n");
    EXPECT_EQ(result, "[%-5X{level}] %-20c: %m%n");
}

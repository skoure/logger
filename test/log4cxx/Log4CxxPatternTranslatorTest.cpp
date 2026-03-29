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
    EXPECT_EQ(Log4CxxPatternTranslator::translate("%p"), "%p");
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
    EXPECT_EQ(result, "[%d{yyyy-MM-dd HH:mm:ss}] [%p] %m%n");
}

TEST(Log4CxxPatternTranslatorTest, MarkerInFullPattern)
{
    std::string result = Log4CxxPatternTranslator::translate(
        "[%p] [%M] %m%n");
    EXPECT_EQ(result, "[%p] [%X{marker}] %m%n");
}

/**
 * @file SpdlogPatternTranslatorTest.cpp
 * @brief Unit tests for SpdlogPatternTranslator.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 */
#include <gtest/gtest.h>
#include <SpdlogPatternTranslator.h>

using namespace sk::logger;

TEST(SpdlogPatternTranslatorTest, MessageToken)
{
    EXPECT_EQ(SpdlogPatternTranslator::translate("%m"), "%v");
}

TEST(SpdlogPatternTranslatorTest, LevelToken)
{
    EXPECT_EQ(SpdlogPatternTranslator::translate("%p"), "%l");
}

TEST(SpdlogPatternTranslatorTest, LoggerNameToken)
{
    EXPECT_EQ(SpdlogPatternTranslator::translate("%c"), "%n");
}

TEST(SpdlogPatternTranslatorTest, ThreadIdTokenPassesThrough)
{
    EXPECT_EQ(SpdlogPatternTranslator::translate("%t"), "%t");
}

TEST(SpdlogPatternTranslatorTest, NewlineToken)
{
    EXPECT_EQ(SpdlogPatternTranslator::translate("%n"), "\n");
}

TEST(SpdlogPatternTranslatorTest, DateTokenStripsWrapper)
{
    // %d{%Y-%m-%d %H:%M:%S} → strftime tokens inlined
    EXPECT_EQ(SpdlogPatternTranslator::translate("%d{%Y-%m-%d %H:%M:%S}"),
              "%Y-%m-%d %H:%M:%S");
}

TEST(SpdlogPatternTranslatorTest, ThreadNameTokenMapsToCustomFlag)
{
    EXPECT_EQ(SpdlogPatternTranslator::translate("%T"), "%*");
}

TEST(SpdlogPatternTranslatorTest, MarkerTokenMapsToCustomFlag)
{
    EXPECT_EQ(SpdlogPatternTranslator::translate("%M"), "%&");
}

TEST(SpdlogPatternTranslatorTest, MultipleTokensInOnePattern)
{
    std::string result = SpdlogPatternTranslator::translate("[%p] %c: %m%n");
    EXPECT_EQ(result, "[%l] %n: %v\n");
}

TEST(SpdlogPatternTranslatorTest, UnknownTokensPassThrough)
{
    // %q is not a canonical token — pass through
    EXPECT_EQ(SpdlogPatternTranslator::translate("%q"), "%q");
}

TEST(SpdlogPatternTranslatorTest, LiteralTextPassesThrough)
{
    EXPECT_EQ(SpdlogPatternTranslator::translate("Hello World"), "Hello World");
}

TEST(SpdlogPatternTranslatorTest, FullPatternWithDate)
{
    std::string result = SpdlogPatternTranslator::translate(
        "[%d{%Y-%m-%d %H:%M:%S}] [%p] %m%n");
    EXPECT_EQ(result, "[%Y-%m-%d %H:%M:%S] [%l] %v\n");
}

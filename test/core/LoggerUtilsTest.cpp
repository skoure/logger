/**
 * @file LoggerUtilsTest.cpp
 * @brief Unit tests for LoggerUtils internal helper functions.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 2026
 */
#include <gtest/gtest.h>
#include <LoggerUtils.h>
#include <stdexcept>

using namespace sk::logger;

TEST(LoggerUtilsTest, IncludesExceptionMessage) {
    std::runtime_error ex("something went wrong");
    std::string result = formatException("operation failed", ex);
    EXPECT_NE(result.find("something went wrong"), std::string::npos);
}

TEST(LoggerUtilsTest, IncludesContextMessage) {
    std::runtime_error ex("disk full");
    std::string result = formatException("write failed", ex);
    EXPECT_NE(result.find("write failed"), std::string::npos);
    EXPECT_NE(result.find("disk full"), std::string::npos);
}

TEST(LoggerUtilsTest, NullMsgOmitsPrefix) {
    std::runtime_error ex("bare error");
    std::string result = formatException(nullptr, ex);
    EXPECT_NE(result.find("bare error"), std::string::npos);
    EXPECT_NE(result[0], ':');
}

TEST(LoggerUtilsTest, IncludesDemangledTypeName) {
    std::runtime_error ex("typed error");
    std::string result = formatException("context", ex);
    EXPECT_NE(result.find("runtime_error"), std::string::npos);
}

TEST(LoggerUtilsTest, ContextMessageSeparatedFromException) {
    std::runtime_error ex("the cause");
    std::string result = formatException("the context", ex);
    // Format should be "the context: <type>: the cause"
    EXPECT_LT(result.find("the context"), result.find("the cause"));
}

#ifdef USE_CPPTRACE
TEST(LoggerUtilsTest, ContainsStacktraceSection) {
    std::runtime_error ex("traced");
    std::string result = formatException("test", ex);
    // cpptrace generate_trace begins with 'Stack trace (most recent call first):'
    EXPECT_NE(result.find("Stack trace (most recent call first):"), std::string::npos);
}

TEST(LoggerUtilsTest, StacktraceContainsFrames) {
    std::runtime_error ex("frames");
    std::string result = formatException("test", ex);
    // cpptrace frames are prefixed with '#'
    EXPECT_NE(result.find('#'), std::string::npos);
}
#endif

TEST(LoggerUtilsTest, JoinMarkerNamesEmpty) {
    std::vector<const Marker*> markers;
    std::string result = joinMarkerNames(markers);
    EXPECT_TRUE(result.empty());
}

TEST(LoggerUtilsTest, JoinMarkerNamesSingle) {
    class TestMarker : public Marker {
    public:
        const std::string& getName() const override { return name; }
        std::string name;
    };

    TestMarker marker;
    marker.name = "single";
    std::vector<const Marker*> markers = { &marker };
    std::string result = joinMarkerNames(markers);
    EXPECT_EQ(result, "single");
}

TEST(LoggerUtilsTest, JoinMarkerNamesMultiple) {
    class TestMarker : public Marker {
    public:
        const std::string& getName() const override { return name; }
        std::string name;
    };

    TestMarker marker1, marker2, marker3;
    marker1.name = "first";
    marker2.name = "second";
    marker3.name = "third";

    std::vector<const Marker*> markers = { &marker1, &marker2, &marker3 };
    std::string result = joinMarkerNames(markers);
    EXPECT_EQ(result, "first, second, third");
}

TEST(LoggerUtilsTest, ReplaceAllNoMatch) {
    std::string text = "Hello world";
    EXPECT_EQ(replaceAll(text, "{suppressed}", "0"), "Hello world");
}

TEST(LoggerUtilsTest, ReplaceAllSingleMatch) {
    std::string text = "Repeats: {suppressed} count";
    EXPECT_EQ(replaceAll(text, "{suppressed}", "42"), "Repeats: 42 count");
}

TEST(LoggerUtilsTest, ReplaceAllMultipleMatches) {
    std::string text = "{suppressed} items ({suppressed} dropped)";
    EXPECT_EQ(replaceAll(text, "{suppressed}", "5"), "5 items (5 dropped)");
}

TEST(LoggerUtilsTest, ReplaceAllEmptyPlaceholderOrInput) {
    EXPECT_EQ(replaceAll("", "{suppressed}", "5"), "");
    EXPECT_EQ(replaceAll("text", "", "5"), "text");
}

static std::string helperFormat(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::string s = formatMessage(fmt, args);
    va_end(args);
    return s;
}

TEST(LoggerUtilsTest, FormatMessageBasic) {
    EXPECT_EQ(helperFormat("Number: %d, string: %s", 123, "test"), "Number: 123, string: test");
    EXPECT_EQ(helperFormat(nullptr), "");
    EXPECT_EQ(helperFormat(""), "");
}

TEST(LoggerUtilsTest, JoinMarkerNamesWithCustomSeparator) {
    class TestMarker : public Marker {
    public:
        const std::string& getName() const override { return name; }
        std::string name;
    };

    TestMarker marker1, marker2;
    marker1.name = "A";
    marker2.name = "B";

    std::vector<const Marker*> markers = { &marker1, &marker2 };
    std::string result = joinMarkerNames(markers, " | ");
    EXPECT_EQ(result, "A | B");
}

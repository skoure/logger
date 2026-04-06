/**
 * @file Log4CxxLoggerTest.cpp
 * @brief Unit tests for Log4CxxLogger functionality.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 */
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <log4cxx/appenderskeleton.h>
#include <log4cxx/patternlayout.h>
#include <Log4CxxBackend.h>
#include <Log4CxxLogger.h>
#include <Log4CxxPatternTranslator.h>
#include <LoggerBase.h>
#include <logger/LevelNames.h>
#include <logger/LoggerFactory.h>
#include <logger/MarkerFactory.h>
#include <stdexcept>
#include <string>

using namespace sk::logger;

class OutputStreamAppender : public log4cxx::AppenderSkeleton {
public:
    std::ostringstream buffer;

    void append(const log4cxx::spi::LoggingEventPtr& event, log4cxx::helpers::Pool&) override {
        buffer << event->getRenderedMessage() << std::endl;
    }

    void close() override {}
    bool requiresLayout() const override { return false; }
};

// Captures the rendered message and the "marker" MDC value from each log event.
class MarkerCapturingAppender : public log4cxx::AppenderSkeleton {
public:
    std::string capturedMessage;
    std::string capturedMarker;

    void append(const log4cxx::spi::LoggingEventPtr& event, log4cxx::helpers::Pool&) override {
        capturedMessage = event->getRenderedMessage();
        log4cxx::LogString val;
        event->getMDC(LOG4CXX_STR("marker"), val);
        capturedMarker = val;
    }

    void close() override {}
    bool requiresLayout() const override { return false; }
};

TEST(Log4CxxLoggerTest, CanCreateLogger) {
    Log4CxxLogger logger("TestLogger");
    EXPECT_EQ(logger.getName(), "TestLogger");
}

TEST(Log4CxxLoggerTest, LevelSetAndGet) {
    Log4CxxLogger logger("TestLogger");
    logger.setLevel(Logger::Level::Debug);
    EXPECT_EQ(logger.getLevel(), Logger::Level::Debug);
}

TEST(Log4CxxLoggerTest, IsEnabledChecks) {
    Log4CxxLogger logger("TestLogger");
    logger.setLevel(Logger::Level::Info);
    EXPECT_TRUE(logger.isInfoEnabled());
    EXPECT_FALSE(logger.isDebugEnabled());
}

TEST(Log4CxxLoggerTest, InfoOutputsExpectedMessage) {
    LoggerPtr logger = sk::logger::LoggerFactory::getInstance().getLogger("TestLoggerOutput");
    Log4CxxLogger* log4cxxLogger = dynamic_cast<Log4CxxLogger*>(logger.get());
    ASSERT_NE(log4cxxLogger, nullptr);

    log4cxx::AppenderPtr appender(new OutputStreamAppender());
    log4cxxLogger->getInternalLogger()->addAppender(appender);

    logger->info("Hello %s %d", "world", 42);

    log4cxxLogger->getInternalLogger()->removeAppender(appender);

    std::string output = dynamic_cast<OutputStreamAppender*>(appender.get())->buffer.str();
    EXPECT_THAT(output, ::testing::HasSubstr("Hello world 42"));
}

TEST(Log4CxxLoggerTest, ExceptionErrorOutputsContextAndMessage) {
    LoggerPtr logger = sk::logger::LoggerFactory::getInstance().getLogger("ExceptionTestLogger");
    Log4CxxLogger* log4cxxLogger = dynamic_cast<Log4CxxLogger*>(logger.get());
    ASSERT_NE(log4cxxLogger, nullptr);

    log4cxx::AppenderPtr appender(new OutputStreamAppender());
    log4cxxLogger->getInternalLogger()->addAppender(appender);
    log4cxxLogger->setLevel(Logger::Level::Error);

    try {
        throw std::runtime_error("db connection lost");
    } catch (const std::exception& ex) {
        logger->error("query failed", ex);
    }

    log4cxxLogger->getInternalLogger()->removeAppender(appender);

    std::string output = dynamic_cast<OutputStreamAppender*>(appender.get())->buffer.str();
    EXPECT_THAT(output, ::testing::HasSubstr("query failed"));
    EXPECT_THAT(output, ::testing::HasSubstr("db connection lost"));
}

TEST(Log4CxxLoggerTest, ExceptionFatalOutputsContextAndMessage) {
    LoggerPtr logger = sk::logger::LoggerFactory::getInstance().getLogger("ExceptionFatalTestLogger");
    Log4CxxLogger* log4cxxLogger = dynamic_cast<Log4CxxLogger*>(logger.get());
    ASSERT_NE(log4cxxLogger, nullptr);

    log4cxx::AppenderPtr appender(new OutputStreamAppender());
    log4cxxLogger->getInternalLogger()->addAppender(appender);
    log4cxxLogger->setLevel(Logger::Level::Fatal);

    std::runtime_error ex("critical system failure");
    logger->fatal("unrecoverable error", ex);

    log4cxxLogger->getInternalLogger()->removeAppender(appender);

    std::string output = dynamic_cast<OutputStreamAppender*>(appender.get())->buffer.str();
    EXPECT_THAT(output, ::testing::HasSubstr("unrecoverable error"));
    EXPECT_THAT(output, ::testing::HasSubstr("critical system failure"));
}

// ---------------------------------------------------------------------------
// Level management API tests
// ---------------------------------------------------------------------------

TEST(Log4CxxLoggerTest, DefaultLevelNotExplicitlySet) {
    Log4CxxLogger fresh("Log4FreshLogger");
    EXPECT_FALSE(fresh.isLevelExplicitlySet());
}

TEST(Log4CxxLoggerTest, SetLevelMarksExplicitAndSyncsBackend) {
    Log4CxxLogger l("Log4SetLevelLogger");
    l.setLevel(Logger::Level::Debug);
    EXPECT_TRUE(l.isLevelExplicitlySet());
    EXPECT_EQ(l.getLevel(), Logger::Level::Debug);
    EXPECT_TRUE(l.isDebugEnabled());
}

TEST(Log4CxxLoggerTest, ClearLevelRevertsToInherited) {
    Log4CxxLogger l("Log4ClearLevelLogger");
    l.setLevel(Logger::Level::Warn);
    EXPECT_TRUE(l.isLevelExplicitlySet());
    l.clearLevel();
    EXPECT_FALSE(l.isLevelExplicitlySet());
    // After clearing, getLevel() returns the effective (inherited) level.
    // log4cxx root logger defaults to DEBUG.
    EXPECT_EQ(l.getLevel(), Logger::Level::Debug);
}

// ---------------------------------------------------------------------------
// Marker overload tests
// ---------------------------------------------------------------------------

TEST(Log4CxxLoggerTest, MarkerStoredInMDCDuringInfoLog)
{
    LoggerPtr logger = sk::logger::LoggerFactory::getInstance().getLogger("Log4MarkerInfo");
    Log4CxxLogger* log4cxxLogger = dynamic_cast<Log4CxxLogger*>(logger.get());
    ASSERT_NE(log4cxxLogger, nullptr);

    auto* capturer = new MarkerCapturingAppender();
    log4cxx::AppenderPtr appender(capturer);
    log4cxxLogger->getInternalLogger()->addAppender(appender);
    log4cxxLogger->setLevel(Logger::Level::Info);

    auto marker = MarkerFactory::getMarker("SQL");
    logger->info(*marker, "select done");

    log4cxxLogger->getInternalLogger()->removeAppender(appender);

    EXPECT_EQ(capturer->capturedMarker, "SQL");
    EXPECT_THAT(capturer->capturedMessage, ::testing::HasSubstr("select done"));
}

TEST(Log4CxxLoggerTest, MarkerStoredInMDCDuringErrorException)
{
    LoggerPtr logger = sk::logger::LoggerFactory::getInstance().getLogger("Log4MarkerError");
    Log4CxxLogger* log4cxxLogger = dynamic_cast<Log4CxxLogger*>(logger.get());
    ASSERT_NE(log4cxxLogger, nullptr);

    auto* capturer = new MarkerCapturingAppender();
    log4cxx::AppenderPtr appender(capturer);
    log4cxxLogger->getInternalLogger()->addAppender(appender);
    log4cxxLogger->setLevel(Logger::Level::Error);

    auto marker = MarkerFactory::getMarker("DB");
    std::runtime_error ex("connection lost");
    logger->error(*marker, "query failed", ex);

    log4cxxLogger->getInternalLogger()->removeAppender(appender);

    EXPECT_EQ(capturer->capturedMarker, "DB");
    EXPECT_THAT(capturer->capturedMessage, ::testing::HasSubstr("query failed"));
}

TEST(Log4CxxLoggerTest, MDCClearedAfterMarkerLog)
{
    LoggerPtr logger = sk::logger::LoggerFactory::getInstance().getLogger("Log4MarkerCleanup");
    Log4CxxLogger* log4cxxLogger = dynamic_cast<Log4CxxLogger*>(logger.get());
    ASSERT_NE(log4cxxLogger, nullptr);

    auto* capturer = new MarkerCapturingAppender();
    log4cxx::AppenderPtr appender(capturer);
    log4cxxLogger->getInternalLogger()->addAppender(appender);
    log4cxxLogger->setLevel(Logger::Level::Info);

    auto marker = MarkerFactory::getMarker("CLEANUP");
    logger->info(*marker, "first message");

    // Plain log — no marker — MDC should have been cleared after the first call
    logger->info("second message");

    log4cxxLogger->getInternalLogger()->removeAppender(appender);

    // capturer holds the last event; marker should be absent on the second call
    EXPECT_TRUE(capturer->capturedMarker.empty());
}

// ---------------------------------------------------------------------------
// Padding tests
//
// Uses a FormattedCapturingAppender (requiresLayout=true) to get the full
// formatted line via PatternLayout::format(), then checks that the canonical
// pattern's width/alignment modifiers produce the expected padded output.
// ---------------------------------------------------------------------------

class FormattedCapturingAppender : public log4cxx::AppenderSkeleton {
public:
    std::string lastLine;
    bool requiresLayout() const override { return true; }
    void close() override {}
    void append(const log4cxx::spi::LoggingEventPtr& event,
                log4cxx::helpers::Pool& pool) override {
        log4cxx::LogString output;
        this->getLayout()->format(output, event, pool);
        lastLine = std::string(output.begin(), output.end());
    }
};

namespace {

static std::string captureLog4CxxLine(const std::string& canonicalPattern,
                                      const std::string& message,
                                      Logger::Level level = Logger::Level::Info,
                                      const Marker* marker = nullptr)
{
    const std::string log4cxxPattern =
        Log4CxxPatternTranslator::translate(canonicalPattern);

    Log4CxxLogger logger("Log4CxxPadding.Capture");
    logger.setLevel(Logger::Level::Trace);
    logger.getInternalLogger()->setAdditivity(false);

    auto* capturer = new FormattedCapturingAppender();
    auto layout = std::make_shared<log4cxx::PatternLayout>(
        log4cxx::LogString(log4cxxPattern.begin(), log4cxxPattern.end()));
    capturer->setLayout(layout);
    log4cxx::helpers::Pool pool;
    capturer->activateOptions(pool);

    log4cxx::AppenderPtr appender(capturer);
    logger.getInternalLogger()->addAppender(appender);

    if (marker) {
        switch (level) {
        case Logger::Level::Fatal: logger.fatal(*marker, "%s", message.c_str()); break;
        case Logger::Level::Error: logger.error(*marker, "%s", message.c_str()); break;
        case Logger::Level::Warn:  logger.warn (*marker, "%s", message.c_str()); break;
        case Logger::Level::Info:  logger.info (*marker, "%s", message.c_str()); break;
        case Logger::Level::Debug: logger.debug(*marker, "%s", message.c_str()); break;
        case Logger::Level::Trace: logger.trace(*marker, "%s", message.c_str()); break;
        }
    } else {
        switch (level) {
        case Logger::Level::Fatal: logger.fatal("%s", message.c_str()); break;
        case Logger::Level::Error: logger.error("%s", message.c_str()); break;
        case Logger::Level::Warn:  logger.warn ("%s", message.c_str()); break;
        case Logger::Level::Info:  logger.info ("%s", message.c_str()); break;
        case Logger::Level::Debug: logger.debug("%s", message.c_str()); break;
        case Logger::Level::Trace: logger.trace("%s", message.c_str()); break;
        }
    }

    logger.getInternalLogger()->removeAppender(appender);
    return capturer->lastLine;
}

} // namespace

TEST(Log4CxxPaddingTest, MarkerLeftAlignedShorterThanWidth)
{
    auto marker = MarkerFactory::getMarker("HI");
    std::string out = captureLog4CxxLine("[%-10M] %m%n", "msg", Logger::Level::Info, marker.get());
    EXPECT_NE(out.find("[HI        ]"), std::string::npos) << "output: " << out;
}

TEST(Log4CxxPaddingTest, MarkerLeftAlignedEmptyIsAllSpaces)
{
    std::string out = captureLog4CxxLine("[%-10M] %m%n", "msg");
    EXPECT_NE(out.find("[          ]"), std::string::npos) << "output: " << out;
}

TEST(Log4CxxPaddingTest, MarkerRightAligned)
{
    auto marker = MarkerFactory::getMarker("HI");
    std::string out = captureLog4CxxLine("[%10M] %m%n", "msg", Logger::Level::Info, marker.get());
    EXPECT_NE(out.find("[        HI]"), std::string::npos) << "output: " << out;
}

TEST(Log4CxxPaddingTest, MarkerExceedsWidthIsNotTruncated)
{
    auto marker = MarkerFactory::getMarker("VERYLONGMARKER");
    std::string out = captureLog4CxxLine("[%-5M] %m%n", "msg", Logger::Level::Info, marker.get());
    EXPECT_NE(out.find("[VERYLONGMARKER]"), std::string::npos) << "output: " << out;
}

TEST(Log4CxxPaddingTest, LevelLeftAligned)
{
    // log4cxx emits levels as e.g. "INFO ", "WARN " — check "INFO " padded to 5
    std::string out = captureLog4CxxLine("[%-5p] %m%n", "msg");
    EXPECT_NE(out.find("[INFO ]"), std::string::npos) << "output: " << out;
}

TEST(Log4CxxPaddingTest, FullPatternMatchesExpectedLayout)
{
    auto marker = MarkerFactory::getMarker("GREET");
    std::string out = captureLog4CxxLine("[%-5p] [%-10M] %m%n", "Hello", Logger::Level::Info, marker.get());
    EXPECT_NE(out.find("[INFO ] [GREET     ] Hello"), std::string::npos)
        << "output: " << out;
}

// ---------------------------------------------------------------------------
// LevelNames customisation tests
// ---------------------------------------------------------------------------

class Log4CxxLevelNamesTest : public ::testing::Test {
protected:
    void TearDown() override {
        // Restore factory defaults so other tests are unaffected.
        LoggerBase::setLevelNames(LevelNames{});
    }
};

TEST_F(Log4CxxLevelNamesTest, DefaultWarnIsWARN)
{
    std::string out = captureLog4CxxLine("[%p] %m%n", "msg", Logger::Level::Warn);
    EXPECT_NE(out.find("[WARN]"), std::string::npos) << "output: " << out;
}

TEST_F(Log4CxxLevelNamesTest, DefaultInfoIsINFO)
{
    std::string out = captureLog4CxxLine("[%p] %m%n", "msg", Logger::Level::Info);
    EXPECT_NE(out.find("[INFO]"), std::string::npos) << "output: " << out;
}

TEST_F(Log4CxxLevelNamesTest, CustomWarnName)
{
    LevelNames names;
    names.warn = "WARNING";
    LoggerBase::setLevelNames(names);

    std::string out = captureLog4CxxLine("[%p] %m%n", "msg", Logger::Level::Warn);
    EXPECT_NE(out.find("[WARNING]"), std::string::npos) << "output: " << out;
}

TEST_F(Log4CxxLevelNamesTest, CustomWarnNameWithPadding)
{
    LevelNames names;
    names.warn = "WARNING";
    LoggerBase::setLevelNames(names);

    std::string out = captureLog4CxxLine("[%-8p] %m%n", "msg", Logger::Level::Warn);
    EXPECT_NE(out.find("[WARNING ]"), std::string::npos) << "output: " << out;
}


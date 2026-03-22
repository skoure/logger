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
#include <Log4CxxLogger.h>
#include <logger/LoggerFactory.h>
#include <stdexcept>

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


/**
 * @file Log4CxxBackendTest.cpp
 * @brief Unit tests for Log4CxxBackend interface contract and factory integration.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#include <gtest/gtest.h>
#include <Log4CxxBackend.h>
#include <Log4CxxLogger.h>
#include <logger/LoggerFactory.h>
#include <logger/Logger.h>
#include <sstream>
#include <log4cxx/appenderskeleton.h>
#include <log4cxx/patternlayout.h>
#include <SinkConfig.h>

using namespace sk::logger;

class Log4CxxBackendTest : public ::testing::Test {
protected:
    Log4CxxBackend backend;
};

TEST_F(Log4CxxBackendTest, CreateLoggerReturnsNonNull) {
    LoggerPtr logger = backend.createLogger("Log4CxxBackend.CreateTest");
    ASSERT_NE(logger, nullptr);
}

TEST_F(Log4CxxBackendTest, CreateLoggerSetsCorrectName) {
    LoggerPtr logger = backend.createLogger("Log4CxxBackend.NameTest");
    auto* concrete = dynamic_cast<Log4CxxLogger*>(logger.get());
    ASSERT_NE(concrete, nullptr);
    EXPECT_EQ(concrete->getName(), "Log4CxxBackend.NameTest");
}

TEST_F(Log4CxxBackendTest, CreateLoggerReturnsLog4CxxLoggerType) {
    LoggerPtr logger = backend.createLogger("Log4CxxBackend.TypeTest");
    EXPECT_NE(dynamic_cast<Log4CxxLogger*>(logger.get()), nullptr);
}


TEST_F(Log4CxxBackendTest, ConfigureWithOstreamWritesFormattedOutput)
{
    LoggerPtr logger = backend.createLogger("Log4Cxx.OStream.Test");
    logger->setLevel(Logger::Level::Trace);

    std::ostringstream oss;
    backend.configureLoggerWithOstream(logger, oss, "[%p] %m%n");

    logger->info("hello log4cxx");
    EXPECT_NE(oss.str().find("hello log4cxx"), std::string::npos)
        << "output: " << oss.str();
}

TEST_F(Log4CxxBackendTest, ConsoleSinkDefaultHasNoColorTokens)
{
    LoggerPtr logger = backend.createLogger("Log4CxxBackend.Console.DefaultNoColor");
    auto* l4 = dynamic_cast<Log4CxxLogger*>(logger.get());
    ASSERT_NE(l4, nullptr);

    SinkConfig sc;
    sc.type    = "console";
    sc.pattern = "[%p] %m%n";

    backend.configureLogger(logger, {sc});

    auto internalLogger = l4->getInternalLogger();
    auto appenders = internalLogger->getAllAppenders();
    ASSERT_EQ(appenders.size(), 1u);

    auto patternLayout = log4cxx::cast<log4cxx::PatternLayout>(appenders[0]->getLayout());
    ASSERT_NE(patternLayout, nullptr);

    log4cxx::LogString ls = patternLayout->getConversionPattern();
    std::string pattern(ls.begin(), ls.end());
    EXPECT_EQ(pattern.find("%Y"), std::string::npos)
        << "Default console (no color property) should not contain %Y";
}

TEST_F(Log4CxxBackendTest, ConsoleSinkColorTrueWrapsPatternWithColorTokens)
{
    LoggerPtr logger = backend.createLogger("Log4CxxBackend.Console.WithColor");
    auto* l4 = dynamic_cast<Log4CxxLogger*>(logger.get());
    ASSERT_NE(l4, nullptr);

    SinkConfig sc;
    sc.type    = "console";
    sc.pattern = "[%p] %m%n";
    sc.properties["color"] = "true";

    backend.configureLogger(logger, {sc});

    auto internalLogger = l4->getInternalLogger();
    auto appenders = internalLogger->getAllAppenders();
    ASSERT_EQ(appenders.size(), 1u);

    auto patternLayout = log4cxx::cast<log4cxx::PatternLayout>(appenders[0]->getLayout());
    ASSERT_NE(patternLayout, nullptr);

    log4cxx::LogString ls = patternLayout->getConversionPattern();
    std::string pattern(ls.begin(), ls.end());
    EXPECT_EQ(pattern.substr(0, 2), "%Y")
        << "color=true should prepend %Y to pattern";
    EXPECT_NE(pattern.find("%y"), std::string::npos)
        << "color=true should append %y to pattern";
}

// ---------------------------------------------------------------------------
// Sink-level threshold (AppenderSkeleton::setThreshold)
// ---------------------------------------------------------------------------

TEST_F(Log4CxxBackendTest, SinkLevelSetsAppenderThreshold)
{
    LoggerPtr logger = backend.createLogger("Log4Cxx.SinkLevel.Set");
    auto* l4 = dynamic_cast<Log4CxxLogger*>(logger.get());
    ASSERT_NE(l4, nullptr);

    SinkConfig sc;
    sc.type    = "console";
    sc.pattern = "[%p] %m%n";
    sc.level   = Logger::Level::Warn;

    backend.configureLogger(logger, {sc});

    auto internalLogger = l4->getInternalLogger();
    auto appenders = internalLogger->getAllAppenders();
    ASSERT_EQ(appenders.size(), 1u);

    auto skel = std::dynamic_pointer_cast<log4cxx::AppenderSkeleton>(appenders[0]);
    ASSERT_NE(skel, nullptr);
    EXPECT_EQ(skel->getThreshold(), log4cxx::Level::getWarn())
        << "SinkConfig.level=WARN should set appender threshold to WARN";
}

TEST_F(Log4CxxBackendTest, SinkLevelNulloptLeavesThresholdUnset)
{
    LoggerPtr logger = backend.createLogger("Log4Cxx.SinkLevel.NoFilter");
    auto* l4 = dynamic_cast<Log4CxxLogger*>(logger.get());
    ASSERT_NE(l4, nullptr);

    SinkConfig sc;
    sc.type    = "console";
    sc.pattern = "[%p] %m%n";
    // sc.level intentionally left as nullopt

    backend.configureLogger(logger, {sc});

    auto internalLogger = l4->getInternalLogger();
    auto appenders = internalLogger->getAllAppenders();
    ASSERT_EQ(appenders.size(), 1u);

    auto skel = std::dynamic_pointer_cast<log4cxx::AppenderSkeleton>(appenders[0]);
    ASSERT_NE(skel, nullptr);
    EXPECT_EQ(skel->getThreshold(), log4cxx::Level::getAll())
        << "Sink with no level threshold should leave appender at ALL (accept everything)";
}

TEST_F(Log4CxxBackendTest, TwoSinksHaveDifferentThresholds)
{
    LoggerPtr logger = backend.createLogger("Log4Cxx.SinkLevel.TwoSinks");
    auto* l4 = dynamic_cast<Log4CxxLogger*>(logger.get());
    ASSERT_NE(l4, nullptr);

    SinkConfig warn_sc;
    warn_sc.type    = "console";
    warn_sc.pattern = "[%p] %m%n";
    warn_sc.level   = Logger::Level::Warn;

    SinkConfig error_sc;
    error_sc.type    = "console";
    error_sc.pattern = "[%p] %m%n";
    error_sc.level   = Logger::Level::Error;

    backend.configureLogger(logger, {warn_sc, error_sc});

    auto internalLogger = l4->getInternalLogger();
    auto appenders = internalLogger->getAllAppenders();
    ASSERT_EQ(appenders.size(), 2u);

    auto skel0 = std::dynamic_pointer_cast<log4cxx::AppenderSkeleton>(appenders[0]);
    auto skel1 = std::dynamic_pointer_cast<log4cxx::AppenderSkeleton>(appenders[1]);
    ASSERT_NE(skel0, nullptr);
    ASSERT_NE(skel1, nullptr);
    EXPECT_EQ(skel0->getThreshold(), log4cxx::Level::getWarn());
    EXPECT_EQ(skel1->getThreshold(), log4cxx::Level::getError());
}

TEST_F(Log4CxxBackendTest, SinkLevelFilteredViaOstream)
{
    LoggerPtr logger = backend.createLogger("Log4Cxx.SinkLevel.OStream");
    logger->setLevel(Logger::Level::Debug);

    std::ostringstream warn_oss;
    std::ostringstream debug_oss;
    backend.configureLoggerWithOstream(logger, warn_oss,  "[%p] %m%n");
    backend.configureLoggerWithOstream(logger, debug_oss, "[%p] %m%n");

    auto internalLogger = dynamic_cast<Log4CxxLogger*>(logger.get())->getInternalLogger();
    auto appenders = internalLogger->getAllAppenders();
    ASSERT_EQ(appenders.size(), 2u);

    auto skel0 = std::dynamic_pointer_cast<log4cxx::AppenderSkeleton>(appenders[0]);
    auto skel1 = std::dynamic_pointer_cast<log4cxx::AppenderSkeleton>(appenders[1]);
    ASSERT_NE(skel0, nullptr);
    ASSERT_NE(skel1, nullptr);
    skel0->setThreshold(log4cxx::Level::getWarn());
    // skel1 left at ALL

    logger->info("info message");

    EXPECT_TRUE(warn_oss.str().empty())
        << "WARN-threshold appender should suppress INFO; got: " << warn_oss.str();
    EXPECT_NE(debug_oss.str().find("info message"), std::string::npos)
        << "No-threshold appender should pass INFO; got: " << debug_oss.str();
}

// Integration: factory returns valid parent and child loggers (Log4cxx owns hierarchy natively)
TEST(Log4CxxBackendFactoryTest, FactoryReturnsValidChildLogger) {
    LoggerFactory& factory = LoggerFactory::getInstance();

    LoggerPtr parent = factory.getLogger("Log4CxxBackendIT.App");
    LoggerPtr child  = factory.getLogger("Log4CxxBackendIT.App.Db");

    ASSERT_NE(parent, nullptr);
    ASSERT_NE(child, nullptr);
    EXPECT_NE(parent, child);
}

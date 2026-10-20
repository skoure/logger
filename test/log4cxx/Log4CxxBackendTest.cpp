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
#include <logger/FilterFactory.h>
#include <logger/MarkerFactory.h>
#include <sstream>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
#include <log4cxx/appenderskeleton.h>
#include <log4cxx/patternlayout.h>
#include <SinkConfig.h>

using namespace sk::logger;

class Log4CxxBackendTest : public ::testing::Test {
protected:
    Log4CxxBackend backend;
    std::vector<std::string> cleanupFiles;

    void TearDown() override {
        if (!cleanupFiles.empty()) {
            LoggerFactory::configureFromJsonString(R"({"loggers":[]})");
            for (const auto& filePath : cleanupFiles) {
                std::remove(filePath.c_str());
            }
            cleanupFiles.clear();
        }
    }

    void registerTempLogFile(const std::string& filePath) {
        cleanupFiles.push_back(filePath);
    }
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

// Integration: factory returns valid parent and child loggers (Log4cxx owns hierarchy natively)
TEST(Log4CxxBackendFactoryTest, FactoryReturnsValidChildLogger) {
    LoggerFactory& factory = LoggerFactory::getInstance();

    LoggerPtr parent = factory.getLogger("Log4CxxBackendIT.App");
    LoggerPtr child  = factory.getLogger("Log4CxxBackendIT.App.Db");

    ASSERT_NE(parent, nullptr);
    ASSERT_NE(child, nullptr);
    EXPECT_NE(parent, child);
}

TEST_F(Log4CxxBackendTest, Integration_ChildInheritsParentSinksWhenAdditivityTrue)
{
    const std::string rootPath = std::string(TEST_CONFIG_DIR) + "/addt_l4c_true.log";
    std::remove(rootPath.c_str());
    registerTempLogFile(rootPath);

    std::string cfg = R"({"loggers":[{"name":"root","level":"INFO","sinks":[{"type":"file","pattern":"%m%n","properties":{"path":")" + rootPath + R"("}}]}]})";
    LoggerFactory::configureFromJsonString(cfg);

    LoggerPtr child = LoggerFactory::getLogger("Log4cxx.InheritTrue.Child");
    ASSERT_NE(child, nullptr);
    child->setLevel(Logger::Level::Info);
    child->setFlushOn(Logger::Level::Info);
    child->info("l4c-true");

    std::ifstream f(rootPath);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("l4c-true"), std::string::npos)
        << "Child should have inherited root sink and written to file";
}

TEST_F(Log4CxxBackendTest, Integration_ChildDoesNotInheritWhenAdditivityFalse)
{
    const std::string rootPath = std::string(TEST_CONFIG_DIR) + "/addt_l4c_false.log";
    std::remove(rootPath.c_str());    
    registerTempLogFile(rootPath);
    
    std::string cfg = R"({"loggers":[{"name":"root","level":"INFO","sinks":[{"type":"file","pattern":"%m%n","properties":{"path":")" + rootPath + R"("}}]},{"name":"Log4cxx.NoInherit.Child","level":"INFO","additivity":false,"sinks":[]}]})";
    LoggerFactory::configureFromJsonString(cfg);

    LoggerPtr child = LoggerFactory::getLogger("Log4cxx.NoInherit.Child");
    ASSERT_NE(child, nullptr);
    child->setLevel(Logger::Level::Info);
    child->setFlushOn(Logger::Level::Info);
    child->info("l4c-false");

    std::ifstream f(rootPath);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content.find("l4c-false"), std::string::npos)
        << "Child should NOT have inherited root sink when additivity=false";
}

// ---------------------------------------------------------------------------
// Marker-based sink filtering (log4cxx)
// ---------------------------------------------------------------------------

TEST_F(Log4CxxBackendTest, MarkerFilterIncludePassesMatchingEvent)
{
    Log4CxxBackend backend;
    LoggerPtr logger = backend.createLogger("Log4Cxx.Marker.Include.Pass");
    logger->setLevel(Logger::Level::Debug);

    const std::string tmpPath = std::string(TEST_CONFIG_DIR) + "/l4c_marker_inc.log";
    std::remove(tmpPath.c_str());
    registerTempLogFile(tmpPath);

    SinkConfig sc;
    sc.type = "file";
    sc.pattern = "%m%n";
    sc.properties["path"] = tmpPath;
    sc.filters = {FilterFactory::createMarkerFilter(
        {MarkerFactory::getMarker("SECURITY")})};

    backend.configureLogger(logger, {sc});

    auto sec = MarkerFactory::getMarker("SECURITY");
    logger->info(*sec, "included message");

    std::ifstream f(tmpPath);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("included message"), std::string::npos)
        << "Include filter should pass events with a matching marker";
}

TEST_F(Log4CxxBackendTest, MarkerFilterIncludeBlocksNonMatchingEvent)
{
    Log4CxxBackend backend;
    LoggerPtr logger = backend.createLogger("Log4Cxx.Marker.Include.Block");
    logger->setLevel(Logger::Level::Debug);

    const std::string tmpPath = std::string(TEST_CONFIG_DIR) + "/l4c_marker_inc_block.log";
    std::remove(tmpPath.c_str());
    registerTempLogFile(tmpPath);

    SinkConfig sc;
    sc.type = "file";
    sc.pattern = "%m%n";
    sc.properties["path"] = tmpPath;
    sc.filters = {FilterFactory::createMarkerFilter(
        {MarkerFactory::getMarker("SECURITY")})};

    backend.configureLogger(logger, {sc});

    auto other = MarkerFactory::getMarker("OTHER");
    logger->info(*other, "blocked message");

    std::ifstream f(tmpPath);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.empty())
        << "Include filter should block events without a matching marker";
}

TEST_F(Log4CxxBackendTest, MarkerFilterIncludeBlocksUnmarkedEvent)
{
    Log4CxxBackend backend;
    LoggerPtr logger = backend.createLogger("Log4Cxx.Marker.Include.Unmarked");
    logger->setLevel(Logger::Level::Debug);

    const std::string tmpPath = std::string(TEST_CONFIG_DIR) + "/l4c_marker_inc_unmarked.log";
    std::remove(tmpPath.c_str());
    registerTempLogFile(tmpPath);

    SinkConfig sc;
    sc.type = "file";
    sc.pattern = "%m%n";
    sc.properties["path"] = tmpPath;
    sc.filters = {FilterFactory::createMarkerFilter(
        {MarkerFactory::getMarker("SECURITY")})};

    backend.configureLogger(logger, {sc});

    logger->info("plain message");

    std::ifstream f(tmpPath);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.empty())
        << "Include filter should block unmarked events";
}

TEST_F(Log4CxxBackendTest, MarkerFilterExcludeBlocksMatchingEvent)
{
    Log4CxxBackend backend;
    LoggerPtr logger = backend.createLogger("Log4Cxx.Marker.Exclude.Block");
    logger->setLevel(Logger::Level::Debug);

    const std::string tmpPath = std::string(TEST_CONFIG_DIR) + "/l4c_marker_exc_block.log";
    std::remove(tmpPath.c_str());
    registerTempLogFile(tmpPath);

    SinkConfig sc;
    sc.type = "file";
    sc.pattern = "%m%n";
    sc.properties["path"] = tmpPath;
    sc.filters = {FilterFactory::createMarkerFilter(
        {MarkerFactory::getMarker("DEBUG")}, FilterMode::Exclude)};

    backend.configureLogger(logger, {sc});

    auto dbg = MarkerFactory::getMarker("DEBUG");
    logger->info(*dbg, "excluded message");

    std::ifstream f(tmpPath);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.empty())
        << "Exclude filter should block events with a matching marker";
}

TEST_F(Log4CxxBackendTest, MarkerFilterExcludePassesUnmarkedEvent)
{
    Log4CxxBackend backend;
    LoggerPtr logger = backend.createLogger("Log4Cxx.Marker.Exclude.Unmarked");
    logger->setLevel(Logger::Level::Debug);

    const std::string tmpPath = std::string(TEST_CONFIG_DIR) + "/l4c_marker_exc_unmarked.log";
    std::remove(tmpPath.c_str());
    registerTempLogFile(tmpPath);

    SinkConfig sc;
    sc.type = "file";
    sc.pattern = "%m%n";
    sc.properties["path"] = tmpPath;
    sc.filters = {FilterFactory::createMarkerFilter(
        {MarkerFactory::getMarker("DEBUG")}, FilterMode::Exclude)};

    backend.configureLogger(logger, {sc});

    logger->info("plain message");

    std::ifstream f(tmpPath);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("plain message"), std::string::npos)
        << "Exclude filter should pass unmarked events";
}

TEST_F(Log4CxxBackendTest, MarkerFilterExcludePassesNonMatchingEvent)
{
    Log4CxxBackend backend;
    LoggerPtr logger = backend.createLogger("Log4Cxx.Marker.Exclude.NonMatch");
    logger->setLevel(Logger::Level::Debug);

    const std::string tmpPath = std::string(TEST_CONFIG_DIR) + "/l4c_marker_exc_nonmatch.log";
    std::remove(tmpPath.c_str());
    registerTempLogFile(tmpPath);

    SinkConfig sc;
    sc.type = "file";
    sc.pattern = "%m%n";
    sc.properties["path"] = tmpPath;
    sc.filters = {FilterFactory::createMarkerFilter(
        {MarkerFactory::getMarker("DEBUG")}, FilterMode::Exclude)};

    backend.configureLogger(logger, {sc});

    auto other = MarkerFactory::getMarker("OTHER");
    logger->info(*other, "other message");

    std::ifstream f(tmpPath);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("other message"), std::string::npos)
        << "Exclude filter should pass events without a matching marker";
}

TEST_F(Log4CxxBackendTest, MultipleMarkerFiltersUseAndSemantics)
{
    LoggerPtr logger = backend.createLogger("Log4Cxx.Marker.MultipleFilters");
    logger->setLevel(Logger::Level::Trace);

    const std::string tmpPath = std::string(TEST_CONFIG_DIR) + "/l4c_marker_multiple.log";
    std::remove(tmpPath.c_str());
    registerTempLogFile(tmpPath);

    SinkConfig sink;
    sink.type = "file";
    sink.pattern = "%m%n";
    sink.properties["path"] = tmpPath;
    sink.filters = {
        FilterFactory::createMarkerFilter({MarkerFactory::getMarker("SECURITY")}),
        FilterFactory::createMarkerFilter({MarkerFactory::getMarker("AUDIT")})
    };
    backend.configureLogger(logger, {sink});

    auto security = MarkerFactory::getMarker("SECURITY");
    auto audit = MarkerFactory::getMarker("AUDIT");
    logger->info(*security, "only one rule passed");
    logger->info({*security, *audit}, "all rules passed");

    std::ifstream file(tmpPath);
    const std::string content((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    EXPECT_EQ(content.find("only one rule passed"), std::string::npos);
    EXPECT_NE(content.find("all rules passed"), std::string::npos);
}

TEST_F(Log4CxxBackendTest, JsonLoggerNameAndRegexFiltersUseEventFields)
{
    const std::string path = std::string(TEST_CONFIG_DIR) + "/log4cxx_json_name_regex.log";
    std::remove(path.c_str());
    registerTempLogFile(path);

    LoggerFactory::configureFromJsonString(R"({
        "loggers": [{
            "name": "root",
            "level": "TRACE",
            "flush_on": "TRACE",
            "sinks": [{
                "type": "file",
                "pattern": "%m%n",
                "properties": {"path": ")" + path + R"("},
                "filters": [
                    {"type": "logger_name", "names": ["Log4Cxx.Filters.Target"]},
                    {"type": "message_regex", "pattern": "timed.?out"}
                ]
            }]
        }]
    })");

    LoggerFactory::getLogger("Log4Cxx.Filters.Target")->info("database timed out");
    LoggerFactory::getLogger("Log4Cxx.Filters.Target")->info("database healthy");
    LoggerFactory::getLogger("Log4Cxx.Filters.Other")->info("database timed out");

    std::ifstream file(path);
    const std::string content((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    EXPECT_EQ(content, "database timed out\n");
}

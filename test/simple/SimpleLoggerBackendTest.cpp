/**
 * @file SimpleLoggerBackendTest.cpp
 * @brief Unit tests for SimpleLoggerBackend interface contract and factory integration.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#include <gtest/gtest.h>
#include <SimpleLoggerBackend.h>
#include <SimpleLogger.h>
#include <logger/LoggerFactory.h>
#include <logger/Logger.h>
#include <logger/MarkerFactory.h>
#include <sstream>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>

using namespace sk::logger;

class SimpleLoggerBackendTest : public ::testing::Test {
protected:
    SimpleLoggerBackend backend;
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

TEST_F(SimpleLoggerBackendTest, CreateLoggerReturnsNonNull) {
    LoggerPtr logger = backend.createLogger("SimpleBackend.CreateTest");
    ASSERT_NE(logger, nullptr);
}

TEST_F(SimpleLoggerBackendTest, CreateLoggerSetsCorrectName) {
    LoggerPtr logger = backend.createLogger("SimpleBackend.NameTest");
    auto* concrete = dynamic_cast<SimpleLogger*>(logger.get());
    ASSERT_NE(concrete, nullptr);
    EXPECT_EQ(concrete->getName(), "SimpleBackend.NameTest");
}

TEST_F(SimpleLoggerBackendTest, CreateLoggerReturnsSimpleLoggerType) {
    LoggerPtr logger = backend.createLogger("SimpleBackend.TypeTest");
    EXPECT_NE(dynamic_cast<SimpleLogger*>(logger.get()), nullptr);
}

TEST_F(SimpleLoggerBackendTest, ImplementsManagedSinkBackend) {
    // SimpleLogger relies on the factory for sink inheritance — must be an IManagedSinkBackend.
    EXPECT_NE(dynamic_cast<IManagedSinkBackend*>(&backend), nullptr);
}

TEST_F(SimpleLoggerBackendTest, ApplyParentSinksIsNoOp) {
    LoggerPtr parent = backend.createLogger("SimpleBackend.NoOp.Parent");
    LoggerPtr child  = backend.createLogger("SimpleBackend.NoOp.Child");
    child->setLevel(Logger::Level::Debug);

    EXPECT_NO_THROW(backend.applyParentSinks(child, parent));

    // applyParentSinks must not change the child's level — level copy is the factory's job
    EXPECT_EQ(child->getLevel(), Logger::Level::Debug);
}

TEST_F(SimpleLoggerBackendTest, ApplyParentSinksNullChildNocrash) {
    LoggerPtr parent = backend.createLogger("SimpleBackend.NullChild.Parent");
    EXPECT_NO_THROW(backend.applyParentSinks(nullptr, parent));
}

TEST_F(SimpleLoggerBackendTest, ApplyParentSinksNullParentNocrash) {
    LoggerPtr child = backend.createLogger("SimpleBackend.NullParent.Child");
    EXPECT_NO_THROW(backend.applyParentSinks(child, nullptr));
}

TEST_F(SimpleLoggerBackendTest, ApplyParentSinksBothNullNocrash) {
    EXPECT_NO_THROW(backend.applyParentSinks(nullptr, nullptr));
}

TEST_F(SimpleLoggerBackendTest, ConfigureWithOstreamWritesFormattedOutput)
{
    LoggerPtr logger = backend.createLogger("Simple.OStream.Test");
    logger->setLevel(Logger::Level::Trace);

    std::ostringstream oss;
    backend.configureLoggerWithOstream(logger, oss, "[%p] %m%n");

    logger->info("hello simple");
    EXPECT_NE(oss.str().find("[INFO] hello simple"), std::string::npos)
        << "output: " << oss.str();
}

TEST_F(SimpleLoggerBackendTest, ConsoleSinkDefaultHasColorFalse)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.Console.DefaultNoColor");
    auto* sl = dynamic_cast<SimpleLogger*>(logger.get());
    ASSERT_NE(sl, nullptr);

    SinkConfig sc;
    sc.type    = "console";
    sc.pattern = "[%p] %m%n";

    backend.configureLogger(logger, {sc});

    const auto& sinks = sl->getSinks();
    ASSERT_EQ(sinks.size(), 1u);
    EXPECT_FALSE(sinks[0].color) << "Default console sink should have color=false";
}

TEST_F(SimpleLoggerBackendTest, ConsoleSinkColorTrueSetsFlag)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.Console.WithColor");
    auto* sl = dynamic_cast<SimpleLogger*>(logger.get());
    ASSERT_NE(sl, nullptr);

    SinkConfig sc;
    sc.type    = "console";
    sc.pattern = "[%p] %m%n";
    sc.properties["color"] = "true";

    backend.configureLogger(logger, {sc});

    const auto& sinks = sl->getSinks();
    ASSERT_EQ(sinks.size(), 1u);
    EXPECT_TRUE(sinks[0].color) << "color=true should set the color flag on the sink";
}

TEST_F(SimpleLoggerBackendTest, ConsoleSinkColorTrueEmitsAnsiCodes)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.Console.AnsiOutput");
    logger->setLevel(Logger::Level::Trace);
    auto* sl = dynamic_cast<SimpleLogger*>(logger.get());
    ASSERT_NE(sl, nullptr);

    std::ostringstream oss;
    backend.configureLoggerWithOstream(logger, oss, "[%p] %m%n");

    // Manually enable color on the captured sink
    auto sinks = sl->getSinks();
    sinks.back().color = true;
    sl->setSinks(std::move(sinks));

    logger->info("colortest");

    EXPECT_NE(oss.str().find("\x1b["), std::string::npos)
        << "color=true should emit ANSI escape codes; output: " << oss.str();
    EXPECT_NE(oss.str().find("\x1b[0m"), std::string::npos)
        << "color=true should emit ANSI reset code; output: " << oss.str();
}

// ---------------------------------------------------------------------------
// Sink-level threshold filtering
// ---------------------------------------------------------------------------

TEST_F(SimpleLoggerBackendTest, SinkLevelThresholdBlocksLowerSeverity)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.SinkLevel.Block");
    logger->setLevel(Logger::Level::Debug);
    auto* sl = dynamic_cast<SimpleLogger*>(logger.get());
    ASSERT_NE(sl, nullptr);

    std::ostringstream warn_sink;
    std::ostringstream debug_sink;
    backend.configureLoggerWithOstream(logger, warn_sink,  "[%p] %m%n");
    backend.configureLoggerWithOstream(logger, debug_sink, "[%p] %m%n");

    auto sinks = sl->getSinks();
    ASSERT_EQ(sinks.size(), 2u);
    sinks[0].level = Logger::Level::Warn;
    sinks[1].level = Logger::Level::Debug;
    sl->setSinks(std::move(sinks));

    logger->info("info message");

    EXPECT_TRUE(warn_sink.str().empty())
        << "WARN-threshold sink should suppress INFO; got: " << warn_sink.str();
    EXPECT_NE(debug_sink.str().find("info message"), std::string::npos)
        << "DEBUG-threshold sink should pass INFO; got: " << debug_sink.str();
}

TEST_F(SimpleLoggerBackendTest, SinkLevelThresholdPassesEqualSeverity)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.SinkLevel.Equal");
    logger->setLevel(Logger::Level::Debug);
    auto* sl = dynamic_cast<SimpleLogger*>(logger.get());
    ASSERT_NE(sl, nullptr);

    std::ostringstream oss;
    backend.configureLoggerWithOstream(logger, oss, "[%p] %m%n");

    auto sinks = sl->getSinks();
    ASSERT_EQ(sinks.size(), 1u);
    sinks[0].level = Logger::Level::Warn;
    sl->setSinks(std::move(sinks));

    logger->warn("warn message");

    EXPECT_NE(oss.str().find("warn message"), std::string::npos)
        << "WARN-threshold sink should pass WARN; got: " << oss.str();
}

TEST_F(SimpleLoggerBackendTest, SinkLevelThresholdPassesHigherSeverity)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.SinkLevel.Higher");
    logger->setLevel(Logger::Level::Debug);
    auto* sl = dynamic_cast<SimpleLogger*>(logger.get());
    ASSERT_NE(sl, nullptr);

    std::ostringstream oss;
    backend.configureLoggerWithOstream(logger, oss, "[%p] %m%n");

    auto sinks = sl->getSinks();
    ASSERT_EQ(sinks.size(), 1u);
    sinks[0].level = Logger::Level::Warn;
    sl->setSinks(std::move(sinks));

    logger->error("error message");

    EXPECT_NE(oss.str().find("error message"), std::string::npos)
        << "WARN-threshold sink should pass ERROR; got: " << oss.str();
}

TEST_F(SimpleLoggerBackendTest, SinkLevelNulloptPassesAll)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.SinkLevel.NoFilter");
    logger->setLevel(Logger::Level::Debug);
    auto* sl = dynamic_cast<SimpleLogger*>(logger.get());
    ASSERT_NE(sl, nullptr);

    std::ostringstream oss;
    backend.configureLoggerWithOstream(logger, oss, "[%p] %m%n");
    // Leave sink.level as nullopt (default)

    logger->debug("debug message");

    EXPECT_NE(oss.str().find("debug message"), std::string::npos)
        << "Sink with no level threshold should pass DEBUG; got: " << oss.str();
}

TEST_F(SimpleLoggerBackendTest, SinkConfigLevelCopiedToSimpleSink)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.SinkLevel.Config");
    auto* sl = dynamic_cast<SimpleLogger*>(logger.get());
    ASSERT_NE(sl, nullptr);

    SinkConfig sc;
    sc.type    = "console";
    sc.pattern = "[%p] %m%n";
    sc.level   = Logger::Level::Error;

    backend.configureLogger(logger, {sc});

    const auto& sinks = sl->getSinks();
    ASSERT_EQ(sinks.size(), 1u);
    ASSERT_TRUE(sinks[0].level.has_value());
    EXPECT_EQ(*sinks[0].level, Logger::Level::Error);
}

// Integration: LoggerFactoryImpl copies parent level to child (SimpleLogger is IManagedSinkBackend)
TEST(SimpleLoggerBackendFactoryTest, ChildInheritsParentLevel) {
    LoggerFactory& factory = LoggerFactory::getInstance();

    LoggerPtr parent = factory.getLogger("SimpleBackendIT.App");
    parent->setLevel(Logger::Level::Warn);

    LoggerPtr child = factory.getLogger("SimpleBackendIT.App.Db");
    EXPECT_EQ(child->getLevel(), Logger::Level::Warn);
}

TEST_F(SimpleLoggerBackendTest, Integration_ChildInheritsParentSinksWhenAdditivityTrue)
{
    const std::string rootPath = std::string(TEST_CONFIG_DIR) + "/addt_root_true.log";
    std::remove(rootPath.c_str());
    registerTempLogFile(rootPath);

    // Configure root with a file sink
    std::string cfg = R"({"loggers":[{"name":"root","level":"INFO","sinks":[{"type":"file","pattern":"%m%n","properties":{"path":")" + rootPath + R"("}}]}]})";
    LoggerFactory::configureFromJsonString(cfg);

    LoggerPtr child = LoggerFactory::getLogger("Integration.InheritTrue.Child");
    ASSERT_NE(child, nullptr);
    child->setLevel(Logger::Level::Info);
    child->setFlushOn(Logger::Level::Info);
    child->info("msg-true");

    std::ifstream f(rootPath);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("msg-true"), std::string::npos)
        << "Child should have inherited root sink and written to file";
}

TEST_F(SimpleLoggerBackendTest, Integration_ChildDoesNotInheritWhenAdditivityFalse)
{
    const std::string rootPath = std::string(TEST_CONFIG_DIR) + "/addt_root_false.log";
    std::remove(rootPath.c_str());
    registerTempLogFile(rootPath);

    // Configure root with a file sink and a child entry with additivity=false
    std::string cfg = R"({"loggers":[{"name":"root","level":"INFO","sinks":[{"type":"file","pattern":"%m%n","properties":{"path":")" + rootPath + R"("}}]},{"name":"Integration.NoInherit.Child","level":"INFO","additivity":false,"sinks":[]}]})";
    LoggerFactory::configureFromJsonString(cfg);

    LoggerPtr child = LoggerFactory::getLogger("Integration.NoInherit.Child");
    ASSERT_NE(child, nullptr);
    child->setLevel(Logger::Level::Info);
    child->setFlushOn(Logger::Level::Info);
    child->info("msg-false");

    std::ifstream f(rootPath);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content.find("msg-false"), std::string::npos)
        << "Child should NOT have inherited root sink when additivity=false";
}

// ---------------------------------------------------------------------------
// Marker-based sink filtering
// ---------------------------------------------------------------------------

TEST_F(SimpleLoggerBackendTest, OstreamFilterIncludesOnlyMatchingMarkers)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.Marker.OstreamFilter");
    logger->setLevel(Logger::Level::Trace);

    std::ostringstream output;
    auto filter = MarkerFactory::createFilter();
    filter->setMarkers({MarkerFactory::getMarker("SECURITY")});
    backend.configureLoggerWithOstream(logger, output, "%m%n", filter);

    auto security = MarkerFactory::getMarker("SECURITY");
    auto other = MarkerFactory::getMarker("OTHER");
    logger->info(*security, "included output");
    logger->info(*other, "filtered output");
    logger->info("unmarked output");

    EXPECT_NE(output.str().find("included output"), std::string::npos);
    EXPECT_EQ(output.str().find("filtered output"), std::string::npos);
    EXPECT_EQ(output.str().find("unmarked output"), std::string::npos);
}

TEST_F(SimpleLoggerBackendTest, MultipleMarkerFiltersUseAndSemantics)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.Marker.MultipleFilters");
    logger->setLevel(Logger::Level::Trace);

    std::ostringstream output;
    backend.configureLoggerWithOstream(logger, output, "%m%n");
    auto* simpleLogger = static_cast<SimpleLogger*>(logger.get());
    auto sinks = simpleLogger->getSinks();
    ASSERT_FALSE(sinks.empty());
    sinks[0].markerFilters = {
        MarkerFilterConfig({"SECURITY"}),
        MarkerFilterConfig({"AUDIT"})
    };
    simpleLogger->setSinks(std::move(sinks));

    auto security = MarkerFactory::getMarker("SECURITY");
    auto audit = MarkerFactory::getMarker("AUDIT");
    logger->info(*security, "only one rule passed");
    logger->info({*security, *audit}, "all rules passed");

    EXPECT_EQ(output.str().find("only one rule passed"), std::string::npos);
    EXPECT_NE(output.str().find("all rules passed"), std::string::npos);
}

TEST_F(SimpleLoggerBackendTest, MarkerFilterIncludePassesMatchingEvent)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.Marker.Include.Pass");
    logger->setLevel(Logger::Level::Debug);

    std::ostringstream oss;
    backend.configureLoggerWithOstream(logger, oss, "[%p] %m%n");
    auto* sl = static_cast<SimpleLogger*>(logger.get());
    auto sinks = sl->getSinks();
    ASSERT_FALSE(sinks.empty());
    sinks[0].markerFilters = {MarkerFilterConfig({"SECURITY"})};
    sl->setSinks(sinks);

    auto sec = MarkerFactory::getMarker("SECURITY");
    logger->info(*sec, "included message");
    EXPECT_NE(oss.str().find("included message"), std::string::npos)
        << "Include filter should pass events with a matching marker";
}

TEST_F(SimpleLoggerBackendTest, MarkerFilterIncludeBlocksNonMatchingEvent)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.Marker.Include.Block");
    logger->setLevel(Logger::Level::Debug);

    std::ostringstream oss;
    backend.configureLoggerWithOstream(logger, oss, "[%p] %m%n");
    auto* sl = static_cast<SimpleLogger*>(logger.get());
    auto sinks = sl->getSinks();
    ASSERT_FALSE(sinks.empty());
    sinks[0].markerFilters = {MarkerFilterConfig({"SECURITY"})};
    sl->setSinks(sinks);

    auto other = MarkerFactory::getMarker("OTHER");
    logger->info(*other, "blocked message");
    EXPECT_TRUE(oss.str().empty())
        << "Include filter should block events without a matching marker";
}

TEST_F(SimpleLoggerBackendTest, MarkerFilterIncludeBlocksUnmarkedEvent)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.Marker.Include.Unmarked");
    logger->setLevel(Logger::Level::Debug);

    std::ostringstream oss;
    backend.configureLoggerWithOstream(logger, oss, "[%p] %m%n");
    auto* sl = static_cast<SimpleLogger*>(logger.get());
    auto sinks = sl->getSinks();
    ASSERT_FALSE(sinks.empty());
    sinks[0].markerFilters = {MarkerFilterConfig({"SECURITY"})};
    sl->setSinks(sinks);

    logger->info("plain message");
    EXPECT_TRUE(oss.str().empty())
        << "Include filter should block unmarked events";
}

TEST_F(SimpleLoggerBackendTest, MarkerFilterIncludePassesWhenAnyMarkerMatches)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.Marker.Include.AnyMatch");
    logger->setLevel(Logger::Level::Debug);

    std::ostringstream oss;
    backend.configureLoggerWithOstream(logger, oss, "[%p] %m%n");
    auto* sl = static_cast<SimpleLogger*>(logger.get());
    auto sinks = sl->getSinks();
    ASSERT_FALSE(sinks.empty());
    sinks[0].markerFilters = {MarkerFilterConfig({"AUDIT", "SECURITY"})};
    sl->setSinks(sinks);

    // Event carries two markers; only one needs to match
    auto audit = MarkerFactory::getMarker("AUDIT");
    auto sql   = MarkerFactory::getMarker("SQL");
    logger->info({*audit, *sql}, "multi-marker message");
    EXPECT_NE(oss.str().find("multi-marker message"), std::string::npos)
        << "Include filter should pass when any event marker matches";
}

TEST_F(SimpleLoggerBackendTest, MarkerFilterExcludeBlocksMatchingEvent)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.Marker.Exclude.Block");
    logger->setLevel(Logger::Level::Debug);

    std::ostringstream oss;
    backend.configureLoggerWithOstream(logger, oss, "[%p] %m%n");
    auto* sl = static_cast<SimpleLogger*>(logger.get());
    auto sinks = sl->getSinks();
    ASSERT_FALSE(sinks.empty());
    sinks[0].markerFilters = {MarkerFilterConfig({"DEBUG"}, true)};
    sl->setSinks(sinks);

    auto dbg = MarkerFactory::getMarker("DEBUG");
    logger->info(*dbg, "excluded message");
    EXPECT_TRUE(oss.str().empty())
        << "Exclude filter should block events with a matching marker";
}

TEST_F(SimpleLoggerBackendTest, MarkerFilterExcludePassesUnmarkedEvent)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.Marker.Exclude.Unmarked");
    logger->setLevel(Logger::Level::Debug);

    std::ostringstream oss;
    backend.configureLoggerWithOstream(logger, oss, "[%p] %m%n");
    auto* sl = static_cast<SimpleLogger*>(logger.get());
    auto sinks = sl->getSinks();
    ASSERT_FALSE(sinks.empty());
    sinks[0].markerFilters = {MarkerFilterConfig({"DEBUG"}, true)};
    sl->setSinks(sinks);

    logger->info("plain message");
    EXPECT_NE(oss.str().find("plain message"), std::string::npos)
        << "Exclude filter should pass unmarked events";
}

TEST_F(SimpleLoggerBackendTest, MarkerFilterExcludePassesNonMatchingEvent)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.Marker.Exclude.NonMatch");
    logger->setLevel(Logger::Level::Debug);

    std::ostringstream oss;
    backend.configureLoggerWithOstream(logger, oss, "[%p] %m%n");
    auto* sl = static_cast<SimpleLogger*>(logger.get());
    auto sinks = sl->getSinks();
    ASSERT_FALSE(sinks.empty());
    sinks[0].markerFilters = {MarkerFilterConfig({"DEBUG"}, true)};
    sl->setSinks(sinks);

    auto other = MarkerFactory::getMarker("OTHER");
    logger->info(*other, "other message");
    EXPECT_NE(oss.str().find("other message"), std::string::npos)
        << "Exclude filter should pass events without a matching marker";
}

TEST_F(SimpleLoggerBackendTest, SinkConfigMarkerFilterCopiedToSimpleSink)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.Marker.ConfigCopy");
    logger->setLevel(Logger::Level::Debug);

    SinkConfig sc;
    sc.type = "console";
    sc.pattern = "[%p] %m%n";
    sc.markerFilters = {MarkerFilterConfig({"SECURITY", "AUDIT"})};

    backend.configureLogger(logger, {sc});

    auto* sl = dynamic_cast<SimpleLogger*>(logger.get());
    ASSERT_NE(sl, nullptr);
    const auto& sinks = sl->getSinks();
    ASSERT_EQ(sinks.size(), 1u);
    ASSERT_EQ(sinks[0].markerFilters.size(), 1u);
    EXPECT_EQ(sinks[0].markerFilters[0].names.size(), 2u);
    EXPECT_EQ(sinks[0].markerFilters[0].names[0], "SECURITY");
    EXPECT_EQ(sinks[0].markerFilters[0].names[1], "AUDIT");
    EXPECT_FALSE(sinks[0].markerFilters[0].exclude);
}

TEST_F(SimpleLoggerBackendTest, MarkerFilterCombinedWithLevelFilter)
{
    LoggerPtr logger = backend.createLogger("SimpleBackend.Marker.WithLevel");
    logger->setLevel(Logger::Level::Debug);

    std::ostringstream oss;
    backend.configureLoggerWithOstream(logger, oss, "[%p] %m%n");
    auto sinks = static_cast<SimpleLogger*>(logger.get())->getSinks();
    ASSERT_FALSE(sinks.empty());

    sinks[0].level = Logger::Level::Warn;
    sinks[0].markerFilters = {MarkerFilterConfig({"SECURITY"})};
    static_cast<SimpleLogger*>(logger.get())->setSinks(sinks);

    // DEBUG level, no marker — blocked by level
    logger->debug("debug no marker");
    EXPECT_TRUE(oss.str().empty())
        << "Level filter should block before marker check";
    oss.str("");

    // WARN level, SECURITY marker — passes level AND matches marker
    auto sec = MarkerFactory::getMarker("SECURITY");
    logger->warn(*sec, "warn with marker");
    EXPECT_NE(oss.str().find("warn with marker"), std::string::npos)
        << "Should pass both level and marker filters";
    oss.str("");

    // WARN level, AUDIT marker (not in filter) — passes level but blocked by marker
    auto audit = MarkerFactory::getMarker("AUDIT");
    logger->warn(*audit, "warn wrong marker");
    EXPECT_TRUE(oss.str().empty())
        << "Should be blocked by include marker filter";
}

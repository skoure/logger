/**
 * @file SpdlogBackendTest.cpp
 * @brief Unit tests for SpdlogBackend interface contract and factory integration.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#include <gtest/gtest.h>
#include <ProxyLogger.h>
#include <SpdlogBackend.h>
#include <SpdlogLogger.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <SinkConfig.h>
#include <logger/LoggerFactory.h>
#include <logger/Logger.h>
#include <logger/MarkerFactory.h>
#include <functional>
#include <sstream>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
#include <TestUtils.h>

using namespace sk::logger;
using sk::logger::test::captureStdout;

class SpdlogBackendTest : public ::testing::Test {
protected:
    SpdlogBackend backend;
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

TEST_F(SpdlogBackendTest, CreateLoggerReturnsNonNull) {
    LoggerPtr logger = backend.createLogger("SpdlogBackend.CreateTest");
    ASSERT_NE(logger, nullptr);
}

TEST_F(SpdlogBackendTest, CreateLoggerSetsCorrectName) {
    LoggerPtr logger = backend.createLogger("SpdlogBackend.NameTest");
    auto* concrete = dynamic_cast<SpdlogLogger*>(logger.get());
    ASSERT_NE(concrete, nullptr);
    EXPECT_EQ(concrete->getName(), "SpdlogBackend.NameTest");
}

TEST_F(SpdlogBackendTest, CreateLoggerReturnsSpdlogLoggerType) {
    LoggerPtr logger = backend.createLogger("SpdlogBackend.TypeTest");
    EXPECT_NE(dynamic_cast<SpdlogLogger*>(logger.get()), nullptr);
}

TEST_F(SpdlogBackendTest, ImplementsManagedSinkBackend) {
    // spdlog relies on the factory for sink inheritance — must be an IManagedSinkBackend.
    EXPECT_NE(dynamic_cast<IManagedSinkBackend*>(&backend), nullptr);
}

TEST_F(SpdlogBackendTest, ApplyParentSinksCopiesSinksToChild) {
    LoggerPtr parentPtr = backend.createLogger("SpdlogBackend.SinkCopy.Parent");
    LoggerPtr childPtr  = backend.createLogger("SpdlogBackend.SinkCopy.Child");

    auto* parent = dynamic_cast<SpdlogLogger*>(parentPtr.get());
    auto* child  = dynamic_cast<SpdlogLogger*>(childPtr.get());
    ASSERT_NE(parent, nullptr);
    ASSERT_NE(child,  nullptr);

    // Add a sentinel sink to the parent's internal spdlog logger
    std::ostringstream oss;
    auto sentinel = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
    parent->getInternalLogger()->sinks().push_back(sentinel);

    backend.applyParentSinks(childPtr, parentPtr);

    auto& childSinks  = child->getInternalLogger()->sinks();
    auto& parentSinks = parent->getInternalLogger()->sinks();

    EXPECT_EQ(childSinks.size(), parentSinks.size());

    bool found = false;
    for (const auto& s : childSinks) {
        if (s == sentinel) { found = true; break; }
    }
    EXPECT_TRUE(found) << "Sentinel sink was not copied to child";
}

TEST_F(SpdlogBackendTest, ApplyParentSinksNullChildNocrash) {
    LoggerPtr parent = backend.createLogger("SpdlogBackend.NullChild.Parent");
    EXPECT_NO_THROW(backend.applyParentSinks(nullptr, parent));
}

TEST_F(SpdlogBackendTest, ApplyParentSinksNullParentNocrash) {
    LoggerPtr child = backend.createLogger("SpdlogBackend.NullParent.Child");
    EXPECT_NO_THROW(backend.applyParentSinks(child, nullptr));
}

TEST_F(SpdlogBackendTest, ApplyParentSinksBothNullNocrash) {
    EXPECT_NO_THROW(backend.applyParentSinks(nullptr, nullptr));
}

TEST_F(SpdlogBackendTest, ConsoleSinkDefaultIsPlainSink)
{
    LoggerPtr logger = backend.createLogger("SpdlogBackend.Console.DefaultNoColor");
    auto* sl = dynamic_cast<SpdlogLogger*>(logger.get());
    ASSERT_NE(sl, nullptr);

    SinkConfig sc;
    sc.type    = "console";
    sc.pattern = "[%p] %m%n";

    backend.configureLogger(logger, {sc});

    auto& sinks = sl->getInternalLogger()->sinks();
    ASSERT_EQ(sinks.size(), 1u);
    EXPECT_NE(
        dynamic_cast<spdlog::sinks::stdout_sink_mt*>(sinks[0].get()),
        nullptr) << "Default console sink should be stdout_sink_mt";
}

TEST_F(SpdlogBackendTest, ConsoleSinkColorFalseUsesPlainSink)
{
    LoggerPtr logger = backend.createLogger("SpdlogBackend.Console.NoColor");
    auto* sl = dynamic_cast<SpdlogLogger*>(logger.get());
    ASSERT_NE(sl, nullptr);

    SinkConfig sc;
    sc.type    = "console";
    sc.pattern = "[%p] %m%n";
    sc.properties["color"] = "false";

    backend.configureLogger(logger, {sc});

    auto& sinks = sl->getInternalLogger()->sinks();
    ASSERT_EQ(sinks.size(), 1u);
    EXPECT_NE(
        dynamic_cast<spdlog::sinks::stdout_sink_mt*>(sinks[0].get()),
        nullptr) << "color=false console sink should be stdout_sink_mt";
}

// ---------------------------------------------------------------------------
// ANSI color tests
// ---------------------------------------------------------------------------

TEST_F(SpdlogBackendTest, ColorConsoleSinkEmitsGreenAnsiCodesAroundInfoLevel)
{
    // Full pipeline: translator wraps %p in %^ / %$ so ansicolor_stdout_sink_mt
    // injects ANSI codes exactly around the level name.
    // color_always=true forces ANSI output even without a TTY (test runner).
    // spdlog default for INFO: green (\x1b[32m), reset (\x1b[m).
    LoggerFactory::configureFromJsonString(R"({
        "loggers": [{
            "name": "AnsiTest.WithLevel",
            "level": "INFO",
            "sinks": [{
                "type": "console",
                "pattern": "[%p] %m%n",
                "properties": { "color": true, "color_always": true }
            }]
        }]
    })");

    LoggerPtr logger = LoggerFactory::getLogger("AnsiTest.WithLevel");
    ASSERT_NE(logger, nullptr);

    std::string output = captureStdout([&]{ logger->info("test"); });

    ASSERT_FALSE(output.empty()) << "Nothing written to stdout";
    EXPECT_NE(output.find("\x1b[32mINFO"), std::string::npos)
        << "Expected green ANSI code (\\x1b[32m) before INFO; got: " << output;
    EXPECT_NE(output.find("INFO\x1b[m"), std::string::npos)
        << "Expected ANSI reset (\\x1b[m) after INFO; got: " << output;
}

// ---------------------------------------------------------------------------
// Sink-level threshold
// ---------------------------------------------------------------------------

TEST_F(SpdlogBackendTest, SinkLevelSetOnSpdlogSink)
{
    LoggerPtr logger = backend.createLogger("SpdlogBackend.SinkLevel.SetTest");
    auto* sl = dynamic_cast<SpdlogLogger*>(logger.get());
    ASSERT_NE(sl, nullptr);

    SinkConfig sc;
    sc.type    = "console";
    sc.pattern = "[%p] %m%n";
    sc.level   = Logger::Level::Warn;

    backend.configureLogger(logger, {sc});

    auto& sinks = sl->getInternalLogger()->sinks();
    ASSERT_EQ(sinks.size(), 1u);
    EXPECT_EQ(sinks[0]->level(), spdlog::level::warn)
        << "SinkConfig.level=WARN should set spdlog sink level to warn";
}

TEST_F(SpdlogBackendTest, SinkLevelNulloptLeavesDefaultTraceLevel)
{
    LoggerPtr logger = backend.createLogger("SpdlogBackend.SinkLevel.NoFilter");
    auto* sl = dynamic_cast<SpdlogLogger*>(logger.get());
    ASSERT_NE(sl, nullptr);

    SinkConfig sc;
    sc.type    = "console";
    sc.pattern = "[%p] %m%n";
    // sc.level intentionally left as nullopt

    backend.configureLogger(logger, {sc});

    auto& sinks = sl->getInternalLogger()->sinks();
    ASSERT_EQ(sinks.size(), 1u);
    EXPECT_EQ(sinks[0]->level(), spdlog::level::trace)
        << "Sink with no level threshold should default to trace (let all through)";
}

TEST_F(SpdlogBackendTest, TwoSinksHaveDifferentLevels)
{
    LoggerPtr logger = backend.createLogger("SpdlogBackend.SinkLevel.TwoSinks");
    auto* sl = dynamic_cast<SpdlogLogger*>(logger.get());
    ASSERT_NE(sl, nullptr);

    SinkConfig warn_sc;
    warn_sc.type    = "console";
    warn_sc.pattern = "[%p] %m%n";
    warn_sc.level   = Logger::Level::Warn;

    SinkConfig debug_sc;
    debug_sc.type    = "console";
    debug_sc.pattern = "[%p] %m%n";
    debug_sc.level   = Logger::Level::Debug;

    backend.configureLogger(logger, {warn_sc, debug_sc});

    auto& sinks = sl->getInternalLogger()->sinks();
    ASSERT_EQ(sinks.size(), 2u);
    EXPECT_EQ(sinks[0]->level(), spdlog::level::warn);
    EXPECT_EQ(sinks[1]->level(), spdlog::level::debug);
}

TEST_F(SpdlogBackendTest, ConfiguredSinkMarkerFilterComposesWithLevelThreshold)
{
    LoggerPtr logger = backend.createLogger("SpdlogBackend.Marker.Level.Configured");
    logger->setLevel(Logger::Level::Trace);

    const std::string path = std::string(TEST_CONFIG_DIR) + "/spdlog_marker_level.log";
    std::remove(path.c_str());
    registerTempLogFile(path);

    SinkConfig sink;
    sink.type = "file";
    sink.pattern = "%m%n";
    sink.properties["path"] = path;
    sink.level = Logger::Level::Warn;
    sink.markerFilter.names = {"SECURITY"};
    backend.configureLogger(logger, {sink});

    auto security = MarkerFactory::getMarker("SECURITY");
    auto other = MarkerFactory::getMarker("OTHER");
    logger->debug(*security, "blocked by level");
    logger->warn(*other, "blocked by marker");
    logger->warn(*security, "accepted by both");
    auto* spdLogger = dynamic_cast<SpdlogLogger*>(logger.get());
    ASSERT_NE(spdLogger, nullptr);
    spdLogger->getInternalLogger()->flush();

    std::ifstream file(path);
    const std::string content((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    EXPECT_EQ(content.find("blocked by level"), std::string::npos);
    EXPECT_EQ(content.find("blocked by marker"), std::string::npos);
    EXPECT_NE(content.find("accepted by both"), std::string::npos);
}

TEST_F(SpdlogBackendTest, SinkLevelJsonConfigFiltersOutput)
{
    LoggerFactory::configureFromJsonString(R"({
        "loggers": [{
            "name": "SpdlogBackend.SinkLevel.Json",
            "level": "DEBUG",
            "sinks": [{
                "type": "console",
                "level": "WARN",
                "pattern": "[%p] %m%n",
                "properties": { "color_always": false }
            }]
        }]
    })");

    LoggerPtr logger = LoggerFactory::getLogger("SpdlogBackend.SinkLevel.Json");
    ASSERT_NE(logger, nullptr);

    auto proxy = std::dynamic_pointer_cast<ProxyLogger>(logger);
    auto* sl = dynamic_cast<SpdlogLogger*>(proxy->getReal().get());
    ASSERT_NE(sl, nullptr);
    auto& sinks = sl->getInternalLogger()->sinks();
    ASSERT_EQ(sinks.size(), 1u);
    EXPECT_EQ(sinks[0]->level(), spdlog::level::warn)
        << "JSON-configured sink with level=WARN should have spdlog warn level";
}

// Integration: LoggerFactoryImpl copies parent level to child (spdlog is IManagedSinkBackend)
TEST(SpdlogBackendFactoryTest, ChildInheritsParentLevel) {
    LoggerFactory& factory = LoggerFactory::getInstance();

    LoggerPtr parent = factory.getLogger("SpdlogBackendIT.App");
    parent->setLevel(Logger::Level::Debug);

    LoggerPtr child = factory.getLogger("SpdlogBackendIT.App.Db");
    EXPECT_EQ(child->getLevel(), Logger::Level::Debug);
}

TEST_F(SpdlogBackendTest, Integration_ChildInheritsParentSinksByDefault)
{
    const std::string rootPath = std::string(TEST_CONFIG_DIR) + "/addt_spdlog_true.log";
    std::remove(rootPath.c_str());
    registerTempLogFile(rootPath);

    std::string cfg = R"({"loggers":[{"name":"root","level":"INFO","sinks":[{"type":"file","pattern":"%m%n","properties":{"path":")" + rootPath + R"("}}]}]})";
    LoggerFactory::configureFromJsonString(cfg);

    LoggerPtr child = LoggerFactory::getLogger("Spdlog.InheritTrue.Child");
    ASSERT_NE(child, nullptr);
    child->setLevel(Logger::Level::Info);
    child->setFlushOn(Logger::Level::Info);
    child->info("spdlog-true");

    std::ifstream f(rootPath);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("spdlog-true"), std::string::npos)
        << "Child should have inherited root sink and written to file";
}

TEST_F(SpdlogBackendTest, Integration_ChildInheritsParentSinksWhenAdditivityTrue)
{
    const std::string rootPath = std::string(TEST_CONFIG_DIR) + "/addt_spdlog_true.log";
    const std::string childPath = std::string(TEST_CONFIG_DIR) + "/addt_spdlog_child.log";
    std::remove(rootPath.c_str());
    registerTempLogFile(rootPath);
    registerTempLogFile(childPath);

    std::string cfg = R"({"loggers":[{"name":"root","level":"INFO","sinks":[{"type":"file","pattern":"%m%n","properties":{"path":")" + rootPath + R"("}}]},{"name":"Spdlog.InheritTrue.Child","level":"INFO","additivity":true,"sinks":[{"type": "file","pattern": "[%d{%Y-%m-%d %H:%M:%S}] [%-7p] [%-10M] [%t] %m%n","level": "INFO","properties": {"path":")" + childPath + R"("}}]}]})";
    LoggerFactory::configureFromJsonString(cfg);

    LoggerPtr child = LoggerFactory::getLogger("Spdlog.InheritTrue.Child");
    ASSERT_NE(child, nullptr);
    child->setLevel(Logger::Level::Info);
    child->setFlushOn(Logger::Level::Info);
    child->info("spdlog-true");

    std::ifstream f(rootPath);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("spdlog-true"), std::string::npos)
        << "Child should have inherited root sink and written to file";
}

TEST_F(SpdlogBackendTest, Integration_ChildDoesNotInheritWhenAdditivityFalse)
{
    const std::string rootPath = std::string(TEST_CONFIG_DIR) + "/addt_spdlog_false.log";
    std::remove(rootPath.c_str());
    registerTempLogFile(rootPath);

    std::string cfg = R"({"loggers":[{"name":"root","level":"INFO","sinks":[{"type":"file","pattern":"%m%n","properties":{"path":")" + rootPath + R"("}}]},{"name":"Spdlog.NoInherit.Child","level":"INFO","additivity":false,"sinks":[]}]})";
    LoggerFactory::configureFromJsonString(cfg);

    LoggerPtr child = LoggerFactory::getLogger("Spdlog.NoInherit.Child");
    ASSERT_NE(child, nullptr);
    child->setLevel(Logger::Level::Info);
    child->setFlushOn(Logger::Level::Info);
    child->info("spdlog-false");

    std::ifstream f(rootPath);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content.find("spdlog-false"), std::string::npos)
        << "Child should NOT have inherited root sink when additivity=false";
}

// ---------------------------------------------------------------------------
// Marker-based sink filtering via spdlog wrapper sink
// ---------------------------------------------------------------------------

TEST_F(SpdlogBackendTest, MarkerFilterIncludePassesMatchingEvent)
{
    LoggerPtr logger = backend.createLogger("Spdlog.Marker.Include.Pass");
    logger->setLevel(Logger::Level::Debug);

    std::ostringstream oss;
    auto filter = MarkerFactory::createFilter();
    filter->setNames({"SECURITY"});
    filter->setMode(MarkerFilter::Mode::Include);

    backend.configureLoggerWithOstream(logger, oss, "%m%n", filter);

    auto sec = MarkerFactory::getMarker("SECURITY");
    logger->info(*sec, "included message");

    EXPECT_NE(oss.str().find("included message"), std::string::npos)
        << "Include filter should pass events with a matching marker";
}

TEST_F(SpdlogBackendTest, MarkerFilterIncludeBlocksNonMatchingEvent)
{
    LoggerPtr logger = backend.createLogger("Spdlog.Marker.Include.Block");
    logger->setLevel(Logger::Level::Debug);

    std::ostringstream oss;
    auto filter = MarkerFactory::createFilter();
    filter->setNames({"SECURITY"});
    filter->setMode(MarkerFilter::Mode::Include);

    backend.configureLoggerWithOstream(logger, oss, "%m%n", filter);

    auto other = MarkerFactory::getMarker("OTHER");
    logger->info(*other, "blocked message");

    EXPECT_TRUE(oss.str().empty())
        << "Include filter should block events without a matching marker";
}

TEST_F(SpdlogBackendTest, MarkerFilterIncludeBlocksUnmarkedEvent)
{
    LoggerPtr logger = backend.createLogger("Spdlog.Marker.Include.Unmarked");
    logger->setLevel(Logger::Level::Debug);

    std::ostringstream oss;
    auto filter = MarkerFactory::createFilter();
    filter->setNames({"SECURITY"});
    filter->setMode(MarkerFilter::Mode::Include);

    backend.configureLoggerWithOstream(logger, oss, "%m%n", filter);

    logger->info("plain message");

    EXPECT_TRUE(oss.str().empty())
        << "Include filter should block unmarked events";
}

TEST_F(SpdlogBackendTest, MarkerFilterExcludeBlocksMatchingEvent)
{
    LoggerPtr logger = backend.createLogger("Spdlog.Marker.Exclude.Block");
    logger->setLevel(Logger::Level::Debug);

    std::ostringstream oss;
    auto filter = MarkerFactory::createFilter();
    filter->setNames({"DEBUG"});
    filter->setMode(MarkerFilter::Mode::Exclude);

    backend.configureLoggerWithOstream(logger, oss, "%m%n", filter);

    auto dbg = MarkerFactory::getMarker("DEBUG");
    logger->info(*dbg, "excluded message");

    EXPECT_TRUE(oss.str().empty())
        << "Exclude filter should block events with a matching marker";
}

TEST_F(SpdlogBackendTest, MarkerFilterExcludePassesUnmarkedEvent)
{
    LoggerPtr logger = backend.createLogger("Spdlog.Marker.Exclude.Unmarked");
    logger->setLevel(Logger::Level::Debug);

    std::ostringstream oss;
    auto filter = MarkerFactory::createFilter();
    filter->setNames({"DEBUG"});
    filter->setMode(MarkerFilter::Mode::Exclude);

    backend.configureLoggerWithOstream(logger, oss, "%m%n", filter);

    logger->info("plain message");

    EXPECT_NE(oss.str().find("plain message"), std::string::npos)
        << "Exclude filter should pass unmarked events";
}

TEST_F(SpdlogBackendTest, MarkerFilterExcludePassesNonMatchingEvent)
{
    LoggerPtr logger = backend.createLogger("Spdlog.Marker.Exclude.NonMatch");
    logger->setLevel(Logger::Level::Debug);

    std::ostringstream oss;
    auto filter = MarkerFactory::createFilter();
    filter->setNames({"DEBUG"});
    filter->setMode(MarkerFilter::Mode::Exclude);

    backend.configureLoggerWithOstream(logger, oss, "%m%n", filter);

    auto other = MarkerFactory::getMarker("OTHER");
    logger->info(*other, "other message");

    EXPECT_NE(oss.str().find("other message"), std::string::npos)
        << "Exclude filter should pass events without a matching marker";
}

TEST_F(SpdlogBackendTest, MarkerFilterCombinedWithLevelFilter)
{
    LoggerPtr logger = backend.createLogger("Spdlog.Marker.WithLevel");
    logger->setLevel(Logger::Level::Debug);

    std::ostringstream oss;
    auto filter = MarkerFactory::createFilter();
    filter->setNames({"SECURITY"});
    filter->setMode(MarkerFilter::Mode::Include);

    backend.configureLoggerWithOstream(logger, oss, "%m%n", filter);

    // Set sink level manually since configureLoggerWithOstream doesn't handle level
    auto* sl = dynamic_cast<SpdlogLogger*>(logger.get());
    if (sl) {
        auto& sinks = sl->getInternalLogger()->sinks();
        if (!sinks.empty()) {
            sinks[0]->set_level(spdlog::level::warn);
        }
    }

    // DEBUG level, SECURITY marker — blocked by level
    logger->debug("debug no marker");

    EXPECT_TRUE(oss.str().empty())
        << "Level filter should block before marker check";

    // WARN level, SECURITY marker — passes level AND matches marker
    auto sec = MarkerFactory::getMarker("SECURITY");
    logger->warn(*sec, "warn with marker");

    EXPECT_NE(oss.str().find("warn with marker"), std::string::npos)
        << "Should pass both level and marker filters";

    // WARN level, AUDIT marker (not in filter) — passes level but blocked by marker
    auto audit = MarkerFactory::getMarker("AUDIT");
    logger->warn(*audit, "warn wrong marker");

    // The stream should still contain "warn with marker"
    // Check that the second message is NOT present
    EXPECT_TRUE(oss.str().find("warn wrong marker") == std::string::npos)
        << "Should be blocked by include marker filter";
}


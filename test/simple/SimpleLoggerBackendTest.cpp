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
#include <sstream>

using namespace sk::logger;

class SimpleLoggerBackendTest : public ::testing::Test {
protected:
    SimpleLoggerBackend backend;
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

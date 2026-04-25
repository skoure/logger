/**
 * @file LoggerConfiguratorTest.cpp
 * @brief Unit tests for LoggerConfigurator.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 */
#include <gtest/gtest.h>
#include <logger/LoggerFactory.h>
#include <logger/Logger.h>
#include <LoggerConfigurator.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace sk::logger;

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST(LoggerConfiguratorTest, LevelFromJsonAppliedToNamedLogger)
{
    LoggerConfigurator::configureFromJsonString(R"({
        "loggers":[{"name":"LogCfgTest.LevelTest","level":"DEBUG","sinks":[]}]
    })");

    LoggerPtr logger = LoggerFactory::getLogger("LogCfgTest.LevelTest");
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->getLevel(), Logger::Level::Debug);
}

TEST(LoggerConfiguratorTest, RootLoggerLevelApplied)
{
    LoggerPtr root = LoggerFactory::getLogger("root");
    ASSERT_NE(root, nullptr);

    LoggerConfigurator::configureFromJsonString(R"({
        "loggers":[{"name":"root","level":"WARN","sinks":[]}]
    })");
    EXPECT_EQ(root->getLevel(), Logger::Level::Warn);
}

TEST(LoggerConfiguratorTest, ConfigureCalledTwiceIsIdempotent)
{
    const std::string cfg = R"({
        "loggers":[{"name":"LogCfgTest.Idem","level":"ERROR","sinks":[]}]
    })";

    EXPECT_NO_THROW(LoggerConfigurator::configureFromJsonString(cfg));
    EXPECT_NO_THROW(LoggerConfigurator::configureFromJsonString(cfg));

    LoggerPtr logger = LoggerFactory::getLogger("LogCfgTest.Idem");
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->getLevel(), Logger::Level::Error);
}

TEST(LoggerConfiguratorTest, ThrowsOnInvalidJson)
{
    EXPECT_THROW(
        LoggerConfigurator::configureFromJsonString("{ not valid json !!!"),
        std::runtime_error);
}

TEST(LoggerConfiguratorTest, PublicApiViaLoggerFactory)
{
    EXPECT_NO_THROW(LoggerFactory::configureFromJsonString(R"({
        "loggers":[{"name":"LogCfgTest.PublicApi","level":"TRACE","sinks":[]}]
    })"));

    LoggerPtr logger = LoggerFactory::getLogger("LogCfgTest.PublicApi");
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->getLevel(), Logger::Level::Trace);
}

// ---------------------------------------------------------------------------
// configureFromJsonFile tests
// ---------------------------------------------------------------------------

TEST(LoggerConfiguratorTest, FileOverloadAppliesConfig)
{
    const std::string path = std::string(TEST_CONFIG_DIR) + "/cfgtest_file_overload.json";
    {
        std::ofstream f(path);
        f << R"({"loggers":[{"name":"LogCfgTest.FileOverload","level":"DEBUG","sinks":[]}]})";
    }

    LoggerConfigurator::configureFromJsonFile(path);
    std::remove(path.c_str());

    LoggerPtr logger = LoggerFactory::getLogger("LogCfgTest.FileOverload");
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->getLevel(), Logger::Level::Debug);
}

TEST(LoggerConfiguratorTest, FileOverloadThrowsOnMissingFile)
{
    EXPECT_THROW(
        LoggerConfigurator::configureFromJsonFile(
            std::string(TEST_CONFIG_DIR) + "/this_does_not_exist_xyz.json"),
        std::runtime_error);
}

TEST(LoggerConfiguratorTest, PublicApiFileOverloadViaLoggerFactory)
{
    const std::string path = std::string(TEST_CONFIG_DIR) + "/cfgtest_factory_file.json";
    {
        std::ofstream f(path);
        f << R"({"loggers":[{"name":"LogCfgTest.FactoryFile","level":"WARN","sinks":[]}]})";
    }

    EXPECT_NO_THROW(LoggerFactory::configureFromJsonFile(path));
    std::remove(path.c_str());

    LoggerPtr logger = LoggerFactory::getLogger("LogCfgTest.FactoryFile");
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->getLevel(), Logger::Level::Warn);
}

// ---------------------------------------------------------------------------
// Helper: replace backslashes with forward slashes for JSON embedding
// ---------------------------------------------------------------------------
static std::string toJsonPath(const std::string& p)
{
    std::string out;
    out.reserve(p.size());
    for (char c : p)
        out += (c == '\\') ? '/' : c;
    return out;
}

TEST(LoggerConfiguratorTest, ReconfigureUpdatesPreexistingChildSinks)
{
    const std::string logPathA = std::string(TEST_CONFIG_DIR) + "/recfg_sink_a.log";
    const std::string logPathB = std::string(TEST_CONFIG_DIR) + "/recfg_sink_b.log";
    std::remove(logPathA.c_str());
    std::remove(logPathB.c_str());

    LoggerPtr child = LoggerFactory::getLogger("RecfgSinkTest.Child");
    ASSERT_NE(child, nullptr);

    LoggerConfigurator::configureFromJsonString(
        R"({"loggers":[{"name":"root","level":"DEBUG","sinks":[)"
        R"({"type":"file","pattern":"%m%n","properties":{"path":")" +
        toJsonPath(logPathA) + R"("}}]}]})");

    child->setFlushOn(Logger::Level::Info);
    child->info("msg-a");

    LoggerConfigurator::configureFromJsonString(
        R"({"loggers":[{"name":"root","level":"DEBUG","sinks":[)"
        R"({"type":"file","pattern":"%m%n","properties":{"path":")" +
        toJsonPath(logPathB) + R"("}}]}]})");

    child->setFlushOn(Logger::Level::Info);

    {
        std::ifstream f(logPathA);
        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
        EXPECT_TRUE(content.find("msg-a") != std::string::npos)
            << "Child should write to file A after first configure";
    }

    child->info("msg-b");

    {
        std::ifstream f(logPathB);
        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
        EXPECT_TRUE(content.find("msg-b") != std::string::npos)
            << "Child should write to file B after reconfigure";
    }

    std::remove(logPathA.c_str());
    std::remove(logPathB.c_str());
}

TEST(LoggerConfiguratorTest, PlaceholderAncestorLevelInheritanceAtCreation)
{
    LoggerConfigurator::configureFromJsonString(
        R"({"loggers":[{"name":"root","level":"WARN","sinks":[]}]})");

    LoggerPtr child = LoggerFactory::getLogger("PlaceholderLvl.Deep.Child");
    ASSERT_NE(child, nullptr);

    EXPECT_EQ(child->getLevel(), Logger::Level::Warn)
        << "Child under auto-created intermediate loggers should inherit root's level";
}

TEST(LoggerConfiguratorTest, PlaceholderAncestorSinkInheritanceAtCreation)
{
    std::ostringstream stream;
    LoggerFactory::configureLoggerWithOstream("root", stream, "[%p] %m%n");

    LoggerPtr child = LoggerFactory::getLogger("PlaceholderSnk.Deep.Child");
    ASSERT_NE(child, nullptr);

    child->info("placeholder-sink-test");
    EXPECT_TRUE(stream.str().find("placeholder-sink-test") != std::string::npos)
        << "Child under auto-created intermediate loggers should inherit root's sink";
}

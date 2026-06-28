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
#include <LoggerBase.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace sk::logger;

class LoggerConfiguratorTest : public ::testing::Test {
protected:
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

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_F(LoggerConfiguratorTest, LevelFromJsonAppliedToNamedLogger)
{
    LoggerConfigurator::configureFromJsonString(R"({
        "loggers":[{"name":"LogCfgTest.LevelTest","level":"DEBUG","sinks":[]}]
    })");

    LoggerPtr logger = LoggerFactory::getLogger("LogCfgTest.LevelTest");
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->getLevel(), Logger::Level::Debug);
}

TEST_F(LoggerConfiguratorTest, RootLoggerLevelApplied)
{
    LoggerPtr root = LoggerFactory::getLogger("root");
    ASSERT_NE(root, nullptr);

    LoggerConfigurator::configureFromJsonString(R"({
        "loggers":[{"name":"root","level":"WARN","sinks":[]}]
    })");
    EXPECT_EQ(root->getLevel(), Logger::Level::Warn);
}

TEST_F(LoggerConfiguratorTest, ConfigureCalledTwiceIsIdempotent)
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

TEST_F(LoggerConfiguratorTest, ThrowsOnInvalidJson)
{
    EXPECT_THROW(
        LoggerConfigurator::configureFromJsonString("{ not valid json !!!"),
        std::runtime_error);
}

TEST_F(LoggerConfiguratorTest, PublicApiViaLoggerFactory)
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

TEST_F(LoggerConfiguratorTest, FileOverloadAppliesConfig)
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

TEST_F(LoggerConfiguratorTest, FileOverloadThrowsOnMissingFile)
{
    EXPECT_THROW(
        LoggerConfigurator::configureFromJsonFile(
            std::string(TEST_CONFIG_DIR) + "/this_does_not_exist_xyz.json"),
        std::runtime_error);
}

TEST_F(LoggerConfiguratorTest, PublicApiFileOverloadViaLoggerFactory)
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

TEST_F(LoggerConfiguratorTest, ReconfigureUpdatesPreexistingChildSinks)
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

    {
        std::ifstream f(logPathA);
        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
        EXPECT_TRUE(content.find("msg-a") != std::string::npos)
            << "Child should write to file A after first configure";
    }

    LoggerConfigurator::configureFromJsonString(
        R"({"loggers":[{"name":"root","level":"DEBUG","sinks":[)"
        R"({"type":"file","pattern":"%m%n","properties":{"path":")" +
        toJsonPath(logPathB) + R"("}}]}]})");


    child->setFlushOn(Logger::Level::Info);
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

TEST_F(LoggerConfiguratorTest, PlaceholderAncestorLevelInheritanceAtCreation)
{
    LoggerConfigurator::configureFromJsonString(
        R"({"loggers":[{"name":"root","level":"WARN","sinks":[]}]})");

    LoggerPtr child = LoggerFactory::getLogger("PlaceholderLvl.Deep.Child");
    ASSERT_NE(child, nullptr);

    EXPECT_EQ(child->getLevel(), Logger::Level::Warn)
        << "Child under auto-created intermediate loggers should inherit root's level";
}

TEST_F(LoggerConfiguratorTest, AdditivityDefaultsToTrue)
{
    // New loggers default to additivity=true
    LoggerPtr l = LoggerFactory::getLogger("CfgAdd.Default");
    ASSERT_NE(l, nullptr);
    {
        auto base = std::dynamic_pointer_cast<LoggerBase>(l);
        ASSERT_NE(base, nullptr);
        EXPECT_TRUE(base->getAdditivity());
    }
}

TEST_F(LoggerConfiguratorTest, AdditivityFromJsonApplied)
{
    LoggerConfigurator::configureFromJsonString(R"({
        "loggers": [
            { "name": "CfgAdd.Explicit", "level": "INFO", "additivity": false, "sinks": [] }
        ]
    })");

    LoggerPtr l = LoggerFactory::getLogger("CfgAdd.Explicit");
    ASSERT_NE(l, nullptr);
    {
        auto base = std::dynamic_pointer_cast<LoggerBase>(l);
        ASSERT_NE(base, nullptr);
        EXPECT_FALSE(base->getAdditivity());
    }
}

TEST_F(LoggerConfiguratorTest, PlaceholderAncestorSinkInheritanceAtCreation)
{
    std::ostringstream stream;
    LoggerFactory::configureLoggerWithOstream("root", stream, "[%p] %m%n");

    LoggerPtr child = LoggerFactory::getLogger("PlaceholderSnk.Deep.Child");
    ASSERT_NE(child, nullptr);

    child->setFlushOn(Logger::Level::Info);
    child->info("placeholder-sink-test");
    EXPECT_TRUE(stream.str().find("placeholder-sink-test") != std::string::npos)
        << "Child under auto-created intermediate loggers should inherit root's sink";
}

TEST_F(LoggerConfiguratorTest, Integration_ChildInheritsParentSinksByDefault)
{
    const std::string rootPath = std::string(TEST_CONFIG_DIR) + "/addt_log_true.log";
    std::remove(rootPath.c_str());
    registerTempLogFile(rootPath);

    std::string cfg = R"({"loggers":[{"name":"root","level":"INFO","sinks":[{"type":"file","pattern":"%m%n","properties":{"path":")" + rootPath + R"("}}]}]})";
    LoggerFactory::configureFromJsonString(cfg);

    LoggerPtr child = LoggerFactory::getLogger("Log.InheritTrue.Child");
    ASSERT_NE(child, nullptr);
    child->setLevel(Logger::Level::Info);
    child->setFlushOn(Logger::Level::Info);
    child->info("log-true");

    std::ifstream f(rootPath);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("log-true"), std::string::npos)
        << "Child should have inherited root sink and written to file";
}

TEST_F(LoggerConfiguratorTest, Integration_ChildInheritsParentSinksWhenAdditivityTrue)
{
    const std::string rootPath = std::string(TEST_CONFIG_DIR) + "/addt_log_true.log";
    const std::string childPath = std::string(TEST_CONFIG_DIR) + "/addt_log_child.log";
    std::remove(rootPath.c_str());
    registerTempLogFile(rootPath);
    registerTempLogFile(childPath);

    std::string cfg = R"({"loggers":[{"name":"root","level":"INFO","sinks":[{"type":"file","pattern":"%m%n","properties":{"path":")" + rootPath + R"("}}]},{"name":"Log.InheritTrue.Child","level":"INFO","additivity":true,"sinks":[{"type": "file","pattern": "[%d{%Y-%m-%d %H:%M:%S}] [%-7p] [%-10M] [%t] %m%n","level": "INFO","properties": {"path":")" + childPath + R"("}}]}]})";
    LoggerFactory::configureFromJsonString(cfg);

    LoggerPtr child = LoggerFactory::getLogger("Log.InheritTrue.Child");
    ASSERT_NE(child, nullptr);
    child->setLevel(Logger::Level::Info);
    child->setFlushOn(Logger::Level::Info);
    child->info("log-true");


    std::ifstream childStream(childPath);
    std::string childContent((std::istreambuf_iterator<char>(childStream)), std::istreambuf_iterator<char>());
    EXPECT_NE(childContent.find("log-true"), std::string::npos)
        << "Child should have written to its configured file";

    std::ifstream parentStream(rootPath);
    std::string parentContent((std::istreambuf_iterator<char>(parentStream)), std::istreambuf_iterator<char>());
    EXPECT_NE(parentContent.find("log-true"), std::string::npos)
        << "Child should have inherited root sink and written to root file";
}

TEST_F(LoggerConfiguratorTest, Integration_ChildDoesNotInheritWhenAdditivityFalse)
{
    const std::string rootPath = std::string(TEST_CONFIG_DIR) + "/addt_log_false.log";
    std::remove(rootPath.c_str());
    registerTempLogFile(rootPath);

    std::string cfg = R"({"loggers":[{"name":"root","level":"INFO","sinks":[{"type":"file","pattern":"%m%n","properties":{"path":")" + rootPath + R"("}}]},{"name":"Log.NoInherit.Child","level":"INFO","additivity":false,"sinks":[]}]})";
    LoggerFactory::configureFromJsonString(cfg);

    LoggerPtr child = LoggerFactory::getLogger("Log.NoInherit.Child");
    ASSERT_NE(child, nullptr);
    child->setLevel(Logger::Level::Info);
    child->setFlushOn(Logger::Level::Info);
    child->info("log-false");

    std::ifstream f(rootPath);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content.find("log-false"), std::string::npos)
        << "Child should NOT have inherited root sink when additivity=false";
}

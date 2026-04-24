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
// Helpers
// ---------------------------------------------------------------------------

// RAII wrapper that deletes the file on destruction, including on test failure.
struct TempFile
{
    std::string path;
    explicit TempFile(std::string p) : path(std::move(p)) {}
    ~TempFile() { if (!path.empty()) std::remove(path.c_str()); }
    TempFile(TempFile&&) = default;
    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
};

// Write a minimal JSON config to a temp file and return a RAII handle.
// TEST_CONFIG_DIR is injected by CMake and points to test/config/ in the
// source tree, which is committed to the repo and always exists.
static TempFile writeTempConfig(const std::string& json)
{
    std::string path = std::string(TEST_CONFIG_DIR) + "/test_logger_config_" +
                       std::to_string(reinterpret_cast<std::uintptr_t>(&json)) + ".json";
    std::FILE* f = std::fopen(path.c_str(), "w");
    if (!f) return TempFile{{}};
    std::fwrite(json.c_str(), 1, json.size(), f);
    std::fclose(f);
    return TempFile{path};
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST(LoggerConfiguratorTest, LevelFromJsonAppliedToNamedLogger)
{
    std::string cfg = R"({
        "loggers":[{"name":"LogCfgTest.LevelTest","level":"DEBUG","sinks":[]}]
    })";
    TempFile tmp = writeTempConfig(cfg);
    ASSERT_FALSE(tmp.path.empty());

    LoggerConfigurator::configure(tmp.path);

    LoggerPtr logger = LoggerFactory::getLogger("LogCfgTest.LevelTest");
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->getLevel(), Logger::Level::Debug);
}

TEST(LoggerConfiguratorTest, RootLoggerLevelApplied)
{
    std::string cfg = R"({
        "loggers":[{"name":"root","level":"WARN","sinks":[]}]
    })";
    TempFile tmp = writeTempConfig(cfg);
    ASSERT_FALSE(tmp.path.empty());

    // Get "root" before configuring so it exists
    LoggerPtr root = LoggerFactory::getLogger("root");
    ASSERT_NE(root, nullptr);

    LoggerConfigurator::configure(tmp.path);
    EXPECT_EQ(root->getLevel(), Logger::Level::Warn);
}

TEST(LoggerConfiguratorTest, ConfigureCalledTwiceIsIdempotent)
{
    std::string cfg = R"({
        "loggers":[{"name":"LogCfgTest.Idem","level":"ERROR","sinks":[]}]
    })";
    TempFile tmp = writeTempConfig(cfg);
    ASSERT_FALSE(tmp.path.empty());

    EXPECT_NO_THROW(LoggerConfigurator::configure(tmp.path));
    EXPECT_NO_THROW(LoggerConfigurator::configure(tmp.path));

    LoggerPtr logger = LoggerFactory::getLogger("LogCfgTest.Idem");
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->getLevel(), Logger::Level::Error);
}

TEST(LoggerConfiguratorTest, ThrowsOnMissingFile)
{
    EXPECT_THROW(
        LoggerConfigurator::configure(std::string(TEST_CONFIG_DIR) + "/this_does_not_exist_xyz.json"),
        std::runtime_error);
}

TEST(LoggerConfiguratorTest, PublicApiViaLoggerFactory)
{
    std::string cfg = R"({
        "loggers":[{"name":"LogCfgTest.PublicApi","level":"TRACE","sinks":[]}]
    })";
    TempFile tmp = writeTempConfig(cfg);
    ASSERT_FALSE(tmp.path.empty());

    EXPECT_NO_THROW(LoggerFactory::configure(tmp.path));

    LoggerPtr logger = LoggerFactory::getLogger("LogCfgTest.PublicApi");
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->getLevel(), Logger::Level::Trace);
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
    // File paths for two successive configure() calls.
    const std::string logPathA = std::string(TEST_CONFIG_DIR) + "/recfg_sink_a.log";
    const std::string logPathB = std::string(TEST_CONFIG_DIR) + "/recfg_sink_b.log";
    std::remove(logPathA.c_str());
    std::remove(logPathB.c_str());

    // Create the child BEFORE any configure call — it will have no sinks yet.
    LoggerPtr child = LoggerFactory::getLogger("RecfgSinkTest.Child");
    ASSERT_NE(child, nullptr);

    // First configure: root → file A.  Phase 3 propagates file A to the child.
    std::string cfgA =
        R"({"loggers":[{"name":"root","level":"DEBUG","sinks":[)"
        R"({"type":"file","pattern":"%m%n","properties":{"path":")" +
        toJsonPath(logPathA) + R"("}}]}]})";
    TempFile tmpA = writeTempConfig(cfgA);
    ASSERT_FALSE(tmpA.path.empty());
    LoggerConfigurator::configure(tmpA.path);

    child->info("msg-a");

    // Second configure: root → file B.  Phase 3 propagates file B to the child.
    // clearAllSinks() in phase 1 destroys the file-A ofstream, flushing buffered data.
    std::string cfgB =
        R"({"loggers":[{"name":"root","level":"DEBUG","sinks":[)"
        R"({"type":"file","pattern":"%m%n","properties":{"path":")" +
        toJsonPath(logPathB) + R"("}}]}]})";
    TempFile tmpB = writeTempConfig(cfgB);
    ASSERT_FALSE(tmpB.path.empty());
    LoggerConfigurator::configure(tmpB.path);

    // File-A sink is now destroyed (clearAllSinks flushed it). Check its content.
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
    // Configure root with WARN level first so there is an explicit level to inherit.
    std::string cfg = R"({"loggers":[{"name":"root","level":"WARN","sinks":[]}]})";
    TempFile tmp = writeTempConfig(cfg);
    ASSERT_FALSE(tmp.path.empty());
    LoggerConfigurator::configure(tmp.path);

    // Create a deep logger. Intermediate ancestors "PlaceholderLvl" and
    // "PlaceholderLvl.Deep" are created as real loggers automatically, so the
    // full parent chain is wired and level walks up to root.
    LoggerPtr child = LoggerFactory::getLogger("PlaceholderLvl.Deep.Child");
    ASSERT_NE(child, nullptr);

    EXPECT_EQ(child->getLevel(), Logger::Level::Warn)
        << "Child under auto-created intermediate loggers should inherit root's level";
}

TEST(LoggerConfiguratorTest, PlaceholderAncestorSinkInheritanceAtCreation)
{
    // Give root an ostream sink before creating the deep child.
    std::ostringstream stream;
    LoggerFactory::configureLoggerWithOstream("root", stream, "[%p] %m%n");

    // Create a deep logger. Intermediate ancestors are created as real loggers
    // automatically, so applyParentSinks() wires root's sink through the chain.
    LoggerPtr child = LoggerFactory::getLogger("PlaceholderSnk.Deep.Child");
    ASSERT_NE(child, nullptr);

    child->info("placeholder-sink-test");
    EXPECT_TRUE(stream.str().find("placeholder-sink-test") != std::string::npos)
        << "Child under auto-created intermediate loggers should inherit root's sink";
}

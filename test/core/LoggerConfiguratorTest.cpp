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

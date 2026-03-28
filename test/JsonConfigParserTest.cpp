/**
 * @file JsonConfigParserTest.cpp
 * @brief Unit tests for JsonConfigParser.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 */
#include <gtest/gtest.h>
#include <JsonConfigParser.h>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace sk::logger;

// Helper: parse from inline string
static std::vector<LoggerConfig> parseString(const std::string& json)
{
    std::istringstream iss(json);
    return JsonConfigParser::parse(iss);
}

// ---------------------------------------------------------------------------
// Basic parsing
// ---------------------------------------------------------------------------

TEST(JsonConfigParserTest, ParsesRootLoggerNameAndLevel)
{
    auto configs = parseString(R"({
        "loggers": [{"name":"root","level":"INFO","sinks":[]}]
    })");
    ASSERT_EQ(configs.size(), 1u);
    EXPECT_EQ(configs[0].name,  "root");
    EXPECT_EQ(configs[0].level, "INFO");
}

TEST(JsonConfigParserTest, ParsesConsoleSinkTypeAndPattern)
{
    auto configs = parseString(R"({
        "loggers": [{
            "name":"root","level":"WARN",
            "sinks":[{"type":"console","pattern":"[%p] %m%n"}]
        }]
    })");
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs[0].sinks.size(), 1u);
    EXPECT_EQ(configs[0].sinks[0].type,    "console");
    EXPECT_EQ(configs[0].sinks[0].pattern, "[%p] %m%n");
}

TEST(JsonConfigParserTest, ParsesFileSinkWithPath)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"App","level":"DEBUG",
            "sinks":[{
                "type":"file","pattern":"[%p] %m",
                "properties":{"path":"logs/app.log"}
            }]
        }]
    })");
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs[0].sinks.size(), 1u);
    const auto& sink = configs[0].sinks[0];
    EXPECT_EQ(sink.type, "file");
    ASSERT_NE(sink.properties.count("path"), 0u);
    EXPECT_EQ(sink.properties.at("path"), "logs/app.log");
}

TEST(JsonConfigParserTest, ParsesRotatingFileSinkProperties)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"App.DB","level":"ERROR",
            "sinks":[{
                "type":"rotating_file",
                "pattern":"[%p] %m",
                "properties":{
                    "path":"logs/db.log",
                    "max_size":"5242880",
                    "max_files":"5"
                }
            }]
        }]
    })");
    ASSERT_EQ(configs.size(), 1u);
    const auto& sink = configs[0].sinks[0];
    EXPECT_EQ(sink.type, "rotating_file");
    EXPECT_EQ(sink.properties.at("path"),      "logs/db.log");
    EXPECT_EQ(sink.properties.at("max_size"),  "5242880");
    EXPECT_EQ(sink.properties.at("max_files"), "5");
}

TEST(JsonConfigParserTest, EmptySinksArrayIsNotAnError)
{
    auto configs = parseString(R"({
        "loggers":[{"name":"App.SQL","level":"TRACE","sinks":[]}]
    })");
    ASSERT_EQ(configs.size(), 1u);
    EXPECT_TRUE(configs[0].sinks.empty());
}

TEST(JsonConfigParserTest, MultipleLoggersPreservesOrder)
{
    auto configs = parseString(R"({
        "loggers":[
            {"name":"root",  "level":"INFO",  "sinks":[]},
            {"name":"App",   "level":"DEBUG", "sinks":[]},
            {"name":"App.DB","level":"WARN",  "sinks":[]}
        ]
    })");
    ASSERT_EQ(configs.size(), 3u);
    EXPECT_EQ(configs[0].name, "root");
    EXPECT_EQ(configs[1].name, "App");
    EXPECT_EQ(configs[2].name, "App.DB");
}

TEST(JsonConfigParserTest, UnknownJsonFieldsAreIgnored)
{
    auto configs = parseString(R"({
        "version":1,
        "loggers":[{
            "name":"root","level":"INFO","sinks":[],
            "extraField":"ignored"
        }]
    })");
    ASSERT_EQ(configs.size(), 1u);
    EXPECT_EQ(configs[0].name,  "root");
    EXPECT_EQ(configs[0].level, "INFO");
}

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

TEST(JsonConfigParserTest, ThrowsOnMissingFile)
{
    EXPECT_THROW(
        JsonConfigParser::parse(std::string(TEST_CONFIG_DIR) + "/this_file_should_not_exist_xyz.json"),
        std::runtime_error);
}

TEST(JsonConfigParserTest, ThrowsOnMalformedJson)
{
    EXPECT_THROW(
        parseString("{ not valid json !!!"),
        std::runtime_error);
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST(JsonConfigParserTest, EmptyLoggersArrayReturnsEmptyVector)
{
    auto configs = parseString(R"({"loggers":[]})");
    EXPECT_TRUE(configs.empty());
}

TEST(JsonConfigParserTest, MissingLoggersKeyReturnsEmptyVector)
{
    auto configs = parseString(R"({})");
    EXPECT_TRUE(configs.empty());
}

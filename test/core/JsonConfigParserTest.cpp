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
#include <logger/LoggerNameFilter.h>
#include <logger/Logger.h>
#include <logger/MarkerFilter.h>
#include <logger/MarkerFactory.h>
#include <logger/RegexFilter.h>
#include <array>
#include <sstream>
#include <string>

using namespace sk::logger;

// Helper: parse from inline string
static std::vector<LoggerConfig> parseString(const std::string& json)
{
    std::istringstream iss(json);
    return JsonConfigParser::parse(iss);
}

static const MarkerFilter& markerFilterAt(const SinkConfig& sink, std::size_t index)
{
    auto filter = std::dynamic_pointer_cast<const MarkerFilter>(sink.filters.at(index));
    if (!filter)
        throw std::runtime_error("Expected a marker filter");
    return *filter;
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
    EXPECT_EQ(configs[0].level, Logger::Level::Info);
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
    EXPECT_EQ(configs[0].level, Logger::Level::Info);
}

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

TEST(JsonConfigParserTest, ThrowsOnMissingFile)
{
    EXPECT_THROW(
        JsonConfigParser::parse(std::string(TEST_CONFIG_DIR) + "/this_file_should_not_exist_xyz.json"),
        ParseException);
}

TEST(JsonConfigParserTest, ThrowsOnMalformedJson)
{
    EXPECT_THROW(
        parseString("{ not valid json !!!"),
        ParseException);
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

// ---------------------------------------------------------------------------
// Property value type tests
// ---------------------------------------------------------------------------

TEST(JsonConfigParserTest, BooleanTruePropertyStoredAsString)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"root","level":"INFO",
            "sinks":[{"type":"console","pattern":"%m","properties":{"color":true}}]
        }]
    })");
    ASSERT_EQ(configs[0].sinks[0].properties.at("color"), "true");
}

TEST(JsonConfigParserTest, BooleanFalsePropertyStoredAsString)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"root","level":"INFO",
            "sinks":[{"type":"console","pattern":"%m","properties":{"color":false}}]
        }]
    })");
    ASSERT_EQ(configs[0].sinks[0].properties.at("color"), "false");
}

TEST(JsonConfigParserTest, IntegerPropertyStoredAsString)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"root","level":"INFO",
            "sinks":[{
                "type":"rotating_file","pattern":"%m",
                "properties":{"path":"app.log","max_size":10485760,"max_files":5}
            }]
        }]
    })");
    const auto& props = configs[0].sinks[0].properties;
    EXPECT_EQ(props.at("max_size"),  "10485760");
    EXPECT_EQ(props.at("max_files"), "5");
}

TEST(JsonConfigParserTest, StringPropertyValueStillWorks)
{
    // Regression: string values must still be accepted after adding bool/int support.
    auto configs = parseString(R"({
        "loggers":[{
            "name":"root","level":"INFO",
            "sinks":[{"type":"console","pattern":"%m","properties":{"color":"true"}}]
        }]
    })");
    ASSERT_EQ(configs[0].sinks[0].properties.at("color"), "true");
}

// ---------------------------------------------------------------------------
// Logger level and flushOn as optional<Logger::Level>
// ---------------------------------------------------------------------------

TEST(JsonConfigParserTest, LoggerLevelParsedAsEnum)
{
    auto configs = parseString(R"({
        "loggers":[{"name":"root","level":"WARN","sinks":[]}]
    })");
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_TRUE(configs[0].level.has_value());
    EXPECT_EQ(*configs[0].level, Logger::Level::Warn);
}

TEST(JsonConfigParserTest, LoggerLevelAbsentIsNullopt)
{
    auto configs = parseString(R"({
        "loggers":[{"name":"root","sinks":[]}]
    })");
    ASSERT_EQ(configs.size(), 1u);
    EXPECT_FALSE(configs[0].level.has_value());
}

TEST(JsonConfigParserTest, AllLoggerLevelStringsMapCorrectly)
{
    struct Case { const char* str; Logger::Level expected; };
    const std::array<Case, 6> cases = {{
        {"FATAL", Logger::Level::Fatal},
        {"ERROR", Logger::Level::Error},
        {"WARN",  Logger::Level::Warn},
        {"INFO",  Logger::Level::Info},
        {"DEBUG", Logger::Level::Debug},
        {"TRACE", Logger::Level::Trace},
    }};
    for (const auto& c : cases)
    {
        std::string json = std::string(R"({"loggers":[{"name":"root","level":")") + c.str + R"(","sinks":[]}]})";
        auto cfgs = parseString(json);
        ASSERT_EQ(cfgs.size(), 1u);
        ASSERT_TRUE(cfgs[0].level.has_value()) << "level=" << c.str;
        EXPECT_EQ(*cfgs[0].level, c.expected) << "level=" << c.str;
    }
}

TEST(JsonConfigParserTest, FlushOnParsedAsEnum)
{
    auto configs = parseString(R"({
        "loggers":[{"name":"root","level":"INFO","flush_on":"ERROR","sinks":[]}]
    })");
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_TRUE(configs[0].flushOn.has_value());
    EXPECT_EQ(*configs[0].flushOn, Logger::Level::Error);
}

TEST(JsonConfigParserTest, UnknownLoggerLevelThrows)
{
    EXPECT_THROW(
        parseString(R"({"loggers":[{"name":"root","level":"INFOO","sinks":[]}]})"),
        ParseException);
}

TEST(JsonConfigParserTest, UnknownSinkLevelThrows)
{
    EXPECT_THROW(
        parseString(R"({
            "loggers":[{
                "name":"root","level":"INFO",
                "sinks":[{"type":"console","level":"INVALID","pattern":"%m"}]
            }]
        })"),
        ParseException);
}

TEST(JsonConfigParserTest, LevelIsCaseInsensitive)
{
    struct Case { const char* str; Logger::Level expected; };
    const std::array<Case, 6> cases = {{
        {"warn",  Logger::Level::Warn},
        {"Warn",  Logger::Level::Warn},
        {"wArN",  Logger::Level::Warn},
        {"fatal", Logger::Level::Fatal},
        {"Debug", Logger::Level::Debug},
        {"trace", Logger::Level::Trace},
    }};
    for (const auto& c : cases)
    {
        std::string json = std::string(R"({"loggers":[{"name":"root","level":")") + c.str + R"(","sinks":[]}]})";
        auto cfgs = parseString(json);
        ASSERT_EQ(cfgs.size(), 1u);
        ASSERT_TRUE(cfgs[0].level.has_value()) << "level=" << c.str;
        EXPECT_EQ(*cfgs[0].level, c.expected) << "level=" << c.str;
    }
}

TEST(JsonConfigParserTest, FlushOnAbsentIsNullopt)
{
    auto configs = parseString(R"({
        "loggers":[{"name":"root","level":"INFO","sinks":[]}]
    })");
    ASSERT_EQ(configs.size(), 1u);
    EXPECT_FALSE(configs[0].flushOn.has_value());
}

// ---------------------------------------------------------------------------
// Sink-level threshold parsing
// ---------------------------------------------------------------------------

TEST(JsonConfigParserTest, SinkLevelParsedAsEnum)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"root","level":"DEBUG",
            "sinks":[{"type":"console","level":"WARN","pattern":"%m"}]
        }]
    })");
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs[0].sinks.size(), 1u);
    ASSERT_TRUE(configs[0].sinks[0].level.has_value());
    EXPECT_EQ(*configs[0].sinks[0].level, Logger::Level::Warn);
}

TEST(JsonConfigParserTest, SinkLevelAbsentIsNullopt)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"root","level":"DEBUG",
            "sinks":[{"type":"console","pattern":"%m"}]
        }]
    })");
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs[0].sinks.size(), 1u);
    EXPECT_FALSE(configs[0].sinks[0].level.has_value());
}

TEST(JsonConfigParserTest, MixedSinkLevels)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"root","level":"DEBUG",
            "sinks":[
                {"type":"console","level":"WARN","pattern":"%m"},
                {"type":"console","pattern":"%m"}
            ]
        }]
    })");
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs[0].sinks.size(), 2u);
    EXPECT_TRUE(configs[0].sinks[0].level.has_value());
    EXPECT_EQ(*configs[0].sinks[0].level, Logger::Level::Warn);
    EXPECT_FALSE(configs[0].sinks[1].level.has_value());
}

// ---------------------------------------------------------------------------
// Marker-based sink filtering parsing
// ---------------------------------------------------------------------------

TEST(JsonConfigParserTest, MarkerFilterArrayParsed)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"root",
            "sinks":[{"type":"console","filters":[{"type":"marker","names":["SECURITY","AUDIT"]}],"pattern":"%m"}]
        }]
    })");
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs[0].sinks.size(), 1u);
    ASSERT_EQ(configs[0].sinks[0].filters.size(), 1u);
    EXPECT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMarkers().size(), 2u);
    EXPECT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMarkers()[0]->getName(), "SECURITY");
    EXPECT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMarkers()[1]->getName(), "AUDIT");
    EXPECT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMode(), FilterMode::Include);
}

TEST(JsonConfigParserTest, FilterArrayParsesMultipleTypedEntries)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"root",
            "sinks":[{"type":"console","filters":[
                {"type":"marker","names":["SECURITY"]},
                {"type":"marker","names":["DEBUG"],"mode":"exclude"}
            ],"pattern":"%m"}]
        }]
    })");

    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs[0].sinks.size(), 1u);
    ASSERT_EQ(configs[0].sinks[0].filters.size(), 2u);
    EXPECT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMarkers()[0]->getName(), "SECURITY");
    EXPECT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMode(), FilterMode::Include);
    EXPECT_EQ(markerFilterAt(configs[0].sinks[0], 1).getMarkers()[0]->getName(), "DEBUG");
    EXPECT_EQ(markerFilterAt(configs[0].sinks[0], 1).getMode(), FilterMode::Exclude);
}

TEST(JsonConfigParserTest, MarkerFilterSingleStringParsed)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"root",
            "sinks":[{"type":"console","filters":[{"type":"marker","names":"SECURITY"}],"pattern":"%m"}]
        }]
    })");
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs[0].sinks.size(), 1u);
    ASSERT_EQ(configs[0].sinks[0].filters.size(), 1u);
    ASSERT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMarkers().size(), 1u);
    EXPECT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMarkers()[0]->getName(), "SECURITY");
}

TEST(JsonConfigParserTest, MarkerFilterWhitespaceTrimmed)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"root",
            "sinks":[{"type":"console","filters":[{"type":"marker","names":[" SECURITY "," AUDIT "]}],"pattern":"%m"}]
        }]
    })");
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs[0].sinks.size(), 1u);
    ASSERT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMarkers().size(), 2u);
    EXPECT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMarkers()[0]->getName(), "SECURITY");
    EXPECT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMarkers()[1]->getName(), "AUDIT");
}

TEST(JsonConfigParserTest, MarkerFilterEmptyNamesSkipped)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"root",
            "sinks":[{"type":"console","filters":[{"type":"marker","names":["SECURITY","","   ","AUDIT"]}],"pattern":"%m"}]
        }]
    })");
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs[0].sinks.size(), 1u);
    ASSERT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMarkers().size(), 2u);
}

TEST(JsonConfigParserTest, MarkerFilterExcludeModeParsed)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"root",
            "sinks":[{"type":"console","filters":[{"type":"marker","names":["DEBUG","TRACE"],"mode":"exclude"}],"pattern":"%m"}]
        }]
    })");
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs[0].sinks.size(), 1u);
    EXPECT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMarkers().size(), 2u);
    EXPECT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMode(), FilterMode::Exclude);
}

TEST(JsonConfigParserTest, MarkerFilterDefaultModeInclude)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"root",
            "sinks":[{"type":"console","filters":[{"type":"marker","names":["DEBUG","TRACE"]}],"pattern":"%m"}]
        }]
    })");
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs[0].sinks.size(), 1u);
    ASSERT_EQ(configs[0].sinks[0].filters.size(), 1u);
    EXPECT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMode(), FilterMode::Include);
}

TEST(JsonConfigParserTest, MarkerFilterUnknownModeThrows)
{
    EXPECT_THROW(parseString(R"({
        "loggers":[{
            "name":"root",
            "sinks":[{"type":"console","filters":[{"type":"marker","names":["DEBUG"],"mode":"unknown"}],"pattern":"%m"}]
        }]
    })"), ParseException);
}

TEST(JsonConfigParserTest, FiltersNodeMustBeArray)
{
    EXPECT_THROW(parseString(R"({
        "loggers":[{
            "name":"root",
            "sinks":[{"type":"console","filters":{},"pattern":"%m"}]
        }]
    })"), ParseException);
}

TEST(JsonConfigParserTest, MarkerFilterNodeMustBeObject)
{
    EXPECT_THROW(parseString(R"({
        "loggers":[{
            "name":"root",
            "sinks":[{"type":"console","filters":["marker"],"pattern":"%m"}]
        }]
    })"), ParseException);
}

TEST(JsonConfigParserTest, FilterEntryRequiresType)
{
    EXPECT_THROW(parseString(R"({
        "loggers":[{
            "name":"root",
            "sinks":[{"type":"console","filters":[{"names":["SECURITY"]}],"pattern":"%m"}]
        }]
    })"), ParseException);
}

TEST(JsonConfigParserTest, UnknownFilterTypeThrows)
{
    EXPECT_THROW(parseString(R"({
        "loggers":[{
            "name":"root",
            "sinks":[{"type":"console","filters":[{"type":"severity"}],"pattern":"%m"}]
        }]
    })"), ParseException);
}

TEST(JsonConfigParserTest, MarkerFilterNamesMustBeStrings)
{
    EXPECT_THROW(parseString(R"({
        "loggers":[{
            "name":"root",
            "sinks":[{"type":"console","filters":[{"type":"marker","names":42}],"pattern":"%m"}]
        }]
    })"), ParseException);
}

TEST(JsonConfigParserTest, MarkerFilterModeMustBeString)
{
    EXPECT_THROW(parseString(R"({
        "loggers":[{
            "name":"root",
            "sinks":[{"type":"console","filters":[{"type":"marker","mode":true}],"pattern":"%m"}]
        }]
    })"), ParseException);
}

TEST(JsonConfigParserTest, MarkerFilterAbsentIsEmpty)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"root",
            "sinks":[{"type":"console","pattern":"%m"}]
        }]
    })");
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs[0].sinks.size(), 1u);
    EXPECT_TRUE(configs[0].sinks[0].filters.empty());
}

TEST(JsonConfigParserTest, EmptyFiltersArrayMeansNoFiltering)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"root",
            "sinks":[{"type":"console","filters":[],"pattern":"%m"}]
        }]
    })");
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs[0].sinks.size(), 1u);
    EXPECT_TRUE(configs[0].sinks[0].filters.empty());
}

TEST(JsonConfigParserTest, MarkerFilterCombinesWithLevel)
{
    auto configs = parseString(R"({
        "loggers":[{
            "name":"root",
            "sinks":[{"type":"file","pattern":"%m","level":"WARN",
                     "filters":[{"type":"marker","names":["SECURITY"],"mode":"exclude"}],
                     "properties":{"path":"/tmp/x.log"}}]
        }]
    })");
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs[0].sinks.size(), 1u);
    ASSERT_TRUE(configs[0].sinks[0].level.has_value());
    EXPECT_EQ(*configs[0].sinks[0].level, Logger::Level::Warn);
    ASSERT_EQ(configs[0].sinks[0].filters.size(), 1u);
    EXPECT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMarkers().size(), 1u);
    EXPECT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMarkers()[0]->getName(), "SECURITY");
    EXPECT_EQ(markerFilterAt(configs[0].sinks[0], 0).getMode(), FilterMode::Exclude);
}

TEST(JsonConfigParserTest, ParsesLoggerNameAndMessageRegexFilters)
{
    auto configs = parseString(R"({"loggers":[{"name":"root","sinks":[{"type":"console","filters":[
        {"type":"logger_name","names":["App.Database"],"mode":"exclude"},
        {"type":"message_regex","pattern":"^timeout","mode":"include"}
    ]}]}]})");

    const auto& filters = configs[0].sinks[0].filters;
    ASSERT_EQ(filters.size(), 2u);
    auto loggerNames = std::dynamic_pointer_cast<const LoggerNameFilter>(filters[0]);
    auto regex = std::dynamic_pointer_cast<const RegexFilter>(filters[1]);
    ASSERT_NE(loggerNames, nullptr);
    ASSERT_NE(regex, nullptr);
    EXPECT_EQ(loggerNames->getNames(), (std::vector<std::string>{"App.Database"}));
    EXPECT_EQ(loggerNames->getMode(), FilterMode::Exclude);
    EXPECT_EQ(regex->getPattern(), "^timeout");
    EXPECT_EQ(regex->getMode(), FilterMode::Include);
}

TEST(JsonConfigParserTest, InvalidMessageRegexThrows)
{
    EXPECT_THROW(parseString(R"({"loggers":[{"name":"root","sinks":[{"type":"console","filters":[
        {"type":"message_regex","pattern":"["}
    ]}]}]})"), ParseException);
}

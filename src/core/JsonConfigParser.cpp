/**
 * @file JsonConfigParser.cpp
 * @brief Parses a JSON logging configuration file into LoggerConfig structs.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 */
#include "JsonConfigParser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

using namespace sk::logger;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string toUpper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return s;
}

static Logger::Level parseLevel(const std::string& s)
{
    const std::string upper = toUpper(s);
    if (upper == "FATAL") return Logger::Level::Fatal;
    if (upper == "ERROR") return Logger::Level::Error;
    if (upper == "WARN")  return Logger::Level::Warn;
    if (upper == "INFO")  return Logger::Level::Info;
    if (upper == "DEBUG") return Logger::Level::Debug;
    if (upper == "TRACE") return Logger::Level::Trace;
    throw ParseException("JsonConfigParser: unknown level '" + s + "'");
}

/**
 * @brief Parses the "markers" field of a sink into a MarkerFilter.
 *
 * Accepts a JSON array of strings, or a single string for convenience.
 * Whitespace around each name is trimmed; empty entries are skipped.
 * Throws ParseException if a marker entry is not a string.
 */
static MarkerFilterConfig parseMarkerFilter(const json& markerNode)
{
    MarkerFilterConfig filter;
    std::vector<std::string> names;

    if (markerNode.is_array())
    {
        for (const auto& entry : markerNode)
        {
            if (!entry.is_string())
                throw ParseException(
                    "JsonConfigParser: 'markers' array entries must be strings");
            names.push_back(entry.get<std::string>());
        }
    }
    else if (markerNode.is_string())
    {
        names.push_back(markerNode.get<std::string>());
    }
    else
    {
        throw ParseException(
            "JsonConfigParser: 'markers' must be a string or an array of strings");
    }

    // Trim whitespace and skip empty entries.
    for (std::string& name : names)
    {
        const auto first = name.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;  // empty entry
        const auto last = name.find_last_not_of(" \t\r\n");
        name = name.substr(first, last - first + 1);
        if (!name.empty())
            filter.names.push_back(std::move(name));
    }

    return filter;
}

/**
 * @brief Parses the "marker_mode" field of a sink.
 *
 * Accepts "include" or "exclude" (case-insensitive).  Throws ParseException
 * for any other value.
 */
static bool parseMarkerMode(const std::string& s)
{
    const std::string upper = toUpper(s);
    if (upper == "EXCLUDE") return true;
    if (upper == "INCLUDE") return false;
    throw ParseException(
        "JsonConfigParser: unknown marker_mode '" + s + "' (expected 'include' or 'exclude')");
}

// ---------------------------------------------------------------------------
// Stream overload — main parsing logic
// ---------------------------------------------------------------------------

std::vector<LoggerConfig> JsonConfigParser::parse(std::istream& stream)
{
    json doc;
    try {
        stream >> doc;
    } catch (const json::parse_error& ex) {
        throw ParseException(
            std::string("JsonConfigParser: malformed JSON: ") + ex.what());
    }

    std::vector<LoggerConfig> result;

    if (!doc.contains("loggers") || !doc["loggers"].is_array())
        return result;

    for (const auto& loggerNode : doc["loggers"])
    {
        LoggerConfig lc;

        if (loggerNode.contains("name") && loggerNode["name"].is_string())
            lc.name = loggerNode["name"].get<std::string>();

        if (loggerNode.contains("level") && loggerNode["level"].is_string())
            lc.level = parseLevel(loggerNode["level"].get<std::string>());

        if (loggerNode.contains("flush_on") && loggerNode["flush_on"].is_string())
            lc.flushOn = parseLevel(loggerNode["flush_on"].get<std::string>());

        if (loggerNode.contains("additivity") && loggerNode["additivity"].is_boolean())
            lc.additivity = loggerNode["additivity"].get<bool>();

        if (loggerNode.contains("sinks") && loggerNode["sinks"].is_array())
        {
            for (const auto& sinkNode : loggerNode["sinks"])
            {
                SinkConfig sc;

                if (sinkNode.contains("type") && sinkNode["type"].is_string())
                    sc.type = sinkNode["type"].get<std::string>();

                if (sinkNode.contains("pattern") && sinkNode["pattern"].is_string())
                    sc.pattern = sinkNode["pattern"].get<std::string>();

                if (sinkNode.contains("level") && sinkNode["level"].is_string())
                    sc.level = parseLevel(sinkNode["level"].get<std::string>());

                if (sinkNode.contains("markers"))
                    sc.markerFilter = parseMarkerFilter(sinkNode["markers"]);

                if (sinkNode.contains("marker_mode") &&
                    sinkNode["marker_mode"].is_string())
                {
                    sc.markerFilter.exclude =
                        parseMarkerMode(sinkNode["marker_mode"].get<std::string>());
                }

                if (sinkNode.contains("properties") && sinkNode["properties"].is_object())
                {
                    for (auto it = sinkNode["properties"].begin();
                         it != sinkNode["properties"].end(); ++it)
                    {
                        if (it.value().is_string())
                            sc.properties[it.key()] = it.value().get<std::string>();
                        else if (it.value().is_boolean())
                            sc.properties[it.key()] = it.value().get<bool>() ? "true" : "false";
                        else if (it.value().is_number_integer() || it.value().is_number_unsigned())
                            sc.properties[it.key()] = std::to_string(it.value().get<int64_t>());
                    }
                }

                lc.sinks.push_back(std::move(sc));
            }
        }

        result.push_back(std::move(lc));
    }

    return result;
}

// ---------------------------------------------------------------------------
// File path overload — delegates to stream overload
// ---------------------------------------------------------------------------

std::vector<LoggerConfig> JsonConfigParser::parse(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        throw ParseException(
            std::string("JsonConfigParser: cannot open file: ") + filePath);

    return parse(file);
}

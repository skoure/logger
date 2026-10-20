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

#include <logger/FilterFactory.h>
#include <logger/MarkerFactory.h>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
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
 * @brief Parses the marker filter mode.
 *
 * Accepts "include" or "exclude" (case-insensitive).
 */
static FilterMode parseFilterMode(const std::string& mode)
{
    const std::string upper = toUpper(mode);
    if (upper == "EXCLUDE") return FilterMode::Exclude;
    if (upper == "INCLUDE") return FilterMode::Include;
    throw ParseException(
        "JsonConfigParser: unknown filter mode '" + mode + "' (expected 'include' or 'exclude')");
}

static std::vector<std::string> parseNames(const json& filterNode,
                                           const std::string& type)
{
    std::vector<std::string> names;

    if (filterNode.contains("names"))
    {
        const json& namesNode = filterNode["names"];
        if (namesNode.is_array())
        {
            for (const auto& entry : namesNode)
            {
                if (!entry.is_string())
                    throw ParseException(
                        "JsonConfigParser: " + type + " filter 'names' array entries must be strings");
                names.push_back(entry.get<std::string>());
            }
        }
        else if (namesNode.is_string())
        {
            names.push_back(namesNode.get<std::string>());
        }
        else
        {
            throw ParseException(
                "JsonConfigParser: " + type + " filter 'names' must be a string or an array of strings");
        }
    }

    std::vector<std::string> trimmedNames;
    trimmedNames.reserve(names.size());
    for (std::string& name : names)
    {
        const auto first = name.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;  // empty entry
        const auto last = name.find_last_not_of(" \t\r\n");
        name = name.substr(first, last - first + 1);
        if (!name.empty())
            trimmedNames.push_back(std::move(name));
    }

    return trimmedNames;
}

static FilterMode parseFilterMode(const json& filterNode)
{
    if (!filterNode.contains("mode"))
        return FilterMode::Include;
    if (!filterNode["mode"].is_string())
        throw ParseException("JsonConfigParser: filter 'mode' must be a string");
    return parseFilterMode(filterNode["mode"].get<std::string>());
}

static std::shared_ptr<const Filter> parseFilter(const json& filterNode)
{
    if (!filterNode.is_object())
        throw ParseException("JsonConfigParser: each 'filters' entry must be an object");
    if (!filterNode.contains("type") || !filterNode["type"].is_string())
        throw ParseException("JsonConfigParser: each filter entry requires a string 'type'");

    const std::string type = filterNode["type"].get<std::string>();
    const FilterMode mode = parseFilterMode(filterNode);

    if (type == "marker")
    {
        std::vector<std::shared_ptr<const Marker>> markers;
        for (const auto& name : parseNames(filterNode, type))
            markers.push_back(MarkerFactory::getMarker(name));
        return FilterFactory::createMarkerFilter(std::move(markers), mode);
    }
    if (type == "logger_name")
        return FilterFactory::createLoggerNameFilter(parseNames(filterNode, type), mode);
    if (type == "message_regex")
    {
        if (!filterNode.contains("pattern") || !filterNode["pattern"].is_string())
            throw ParseException("JsonConfigParser: message_regex filter requires a string 'pattern'");
        try
        {
            return FilterFactory::createRegexFilter(
                filterNode["pattern"].get<std::string>(), mode);
        }
        catch (const std::regex_error& ex)
        {
            throw ParseException(
                std::string("JsonConfigParser: invalid message_regex pattern: ") + ex.what());
        }
    }
    throw ParseException("JsonConfigParser: unknown filter type '" + type + "'");
}

static std::vector<std::shared_ptr<const Filter>> parseFilters(const json& filtersNode)
{
    if (!filtersNode.is_array())
        throw ParseException("JsonConfigParser: 'filters' must be an array");

    std::vector<std::shared_ptr<const Filter>> filters;
    filters.reserve(filtersNode.size());
    for (const auto& filterNode : filtersNode)
        filters.push_back(parseFilter(filterNode));
    return filters;
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

                if (sinkNode.contains("filters"))
                    sc.filters = parseFilters(sinkNode["filters"]);

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

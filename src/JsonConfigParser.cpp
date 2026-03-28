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

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

using namespace sk::logger;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Stream overload — main parsing logic
// ---------------------------------------------------------------------------

std::vector<LoggerConfig> JsonConfigParser::parse(std::istream& stream)
{
    json doc;
    try {
        stream >> doc;
    } catch (const json::parse_error& ex) {
        throw std::runtime_error(
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
            lc.level = loggerNode["level"].get<std::string>();

        if (loggerNode.contains("sinks") && loggerNode["sinks"].is_array())
        {
            for (const auto& sinkNode : loggerNode["sinks"])
            {
                SinkConfig sc;

                if (sinkNode.contains("type") && sinkNode["type"].is_string())
                    sc.type = sinkNode["type"].get<std::string>();

                if (sinkNode.contains("pattern") && sinkNode["pattern"].is_string())
                    sc.pattern = sinkNode["pattern"].get<std::string>();

                if (sinkNode.contains("properties") && sinkNode["properties"].is_object())
                {
                    for (auto it = sinkNode["properties"].begin();
                         it != sinkNode["properties"].end(); ++it)
                    {
                        if (it.value().is_string())
                            sc.properties[it.key()] = it.value().get<std::string>();
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
        throw std::runtime_error(
            std::string("JsonConfigParser: cannot open file: ") + filePath);

    return parse(file);
}

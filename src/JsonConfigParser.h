/**
 * @file JsonConfigParser.h
 * @brief Parses a JSON logging configuration file into LoggerConfig structs.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 */
#ifndef SK_JSON_CONFIG_PARSER_H
#define SK_JSON_CONFIG_PARSER_H

#include <LoggerConfig.h>
#include <istream>
#include <string>
#include <vector>

namespace sk { namespace logger {

/**
 * @class JsonConfigParser
 * @brief Parses a JSON logging configuration into a vector of LoggerConfig.
 *
 * Expects the JSON document to have the structure:
 * @code
 * {
 *   "loggers": [
 *     {
 *       "name":  "root",
 *       "level": "INFO",
 *       "sinks": [
 *         {
 *           "type":    "console",
 *           "pattern": "[%d{%Y-%m-%d %H:%M:%S}] [%p] %m%n",
 *           "properties": { "path": "app.log" }
 *         }
 *       ]
 *     }
 *   ]
 * }
 * @endcode
 *
 * All methods are static.  Throws std::runtime_error on parse or I/O errors.
 */
class JsonConfigParser
{
public:
    /**
     * @brief Parse a JSON config from a file path.
     *
     * Delegates to the stream overload after opening the file.
     * Throws std::runtime_error if the file cannot be opened.
     *
     * @param filePath Path to the JSON configuration file.
     * @return Ordered vector of LoggerConfig parsed from the file.
     */
    static std::vector<LoggerConfig> parse(const std::string& filePath);

    /**
     * @brief Parse a JSON config from an input stream.
     *
     * Throws std::runtime_error on malformed JSON.
     *
     * @param stream Readable stream containing JSON data.
     * @return Ordered vector of LoggerConfig parsed from the stream.
     */
    static std::vector<LoggerConfig> parse(std::istream& stream);

private:
    JsonConfigParser() = delete;
    ~JsonConfigParser() = delete;
};

}} // namespace sk::logger

#endif // SK_JSON_CONFIG_PARSER_H

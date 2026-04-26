/**
 * @file SinkConfig.h
 * @brief Internal configuration struct for a single logging sink.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 */
#ifndef SK_SINK_CONFIG_H
#define SK_SINK_CONFIG_H

#include <logger/Logger.h>
#include <map>
#include <optional>
#include <string>

namespace sk { namespace logger {

/**
 * @struct SinkConfig
 * @brief Describes a single output sink for a logger.
 *
 * Used internally by JsonConfigParser, LoggerConfigurator, and backend
 * configureLogger() implementations.  Not part of the public API.
 */
struct SinkConfig
{
    /** Sink type: "console", "file", or "rotating_file". */
    std::string type;

    /** Canonical log4j-style pattern, e.g. "[%d{%Y-%m-%d %H:%M:%S}] [%p] %m%n". */
    std::string pattern;

    /** Minimum severity for this sink. nullopt = no additional filtering beyond the logger level. */
    std::optional<Logger::Level> level;

    /** Extra properties (path, max_size, max_files, …). */
    std::map<std::string, std::string> properties;
};

}} // namespace sk::logger

#endif // SK_SINK_CONFIG_H

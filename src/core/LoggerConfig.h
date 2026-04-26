/**
 * @file LoggerConfig.h
 * @brief Internal configuration struct for a single named logger.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 */
#ifndef SK_LOGGER_CONFIG_H
#define SK_LOGGER_CONFIG_H

#include <SinkConfig.h>
#include <logger/Logger.h>
#include <optional>
#include <string>
#include <vector>

namespace sk { namespace logger {

/**
 * @struct LoggerConfig
 * @brief Describes the configuration for one named logger (or "root").
 *
 * Produced by JsonConfigParser and consumed by LoggerConfigurator.
 * Not part of the public API.
 */
struct LoggerConfig
{
    /** Logger name, e.g. "root", "App", "App.Database". */
    std::string name;

    /** Explicit level for this logger. nullopt if absent from JSON. */
    std::optional<Logger::Level> level;

    /** Flush-on threshold. nullopt if absent from JSON. */
    std::optional<Logger::Level> flushOn;

    /** Ordered list of sinks to configure on this logger. */
    std::vector<SinkConfig> sinks;
};

}} // namespace sk::logger

#endif // SK_LOGGER_CONFIG_H

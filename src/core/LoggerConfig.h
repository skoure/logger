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

    /** Raw level string from JSON: "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL". */
    std::string level;

    /** Ordered list of sinks to configure on this logger. */
    std::vector<SinkConfig> sinks;
};

}} // namespace sk::logger

#endif // SK_LOGGER_CONFIG_H

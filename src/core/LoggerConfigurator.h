/**
 * @file LoggerConfigurator.h
 * @brief Drives the full configuration pipeline: JSON → LoggerConfig → backend.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 */
#ifndef SK_LOGGER_CONFIGURATOR_H
#define SK_LOGGER_CONFIGURATOR_H

#include <string>

namespace sk { namespace logger {

/**
 * @class LoggerConfigurator
 * @brief Applies a JSON configuration to the active backend.
 *
 * Call chain:
 * @code
 *   LoggerFactory::configureFromJsonString(json)
 *     -> LoggerConfigurator::configureFromJsonString(json)
 *          -> JsonConfigParser::parse(istream)
 *          -> for each LoggerConfig:
 *               logger = LoggerFactoryImpl::getLogger(name)
 *               logger->setLevel(...)
 *               LoggerFactoryImpl::configureLogger(logger, sinks)
 *                 -> m_backend->configureLogger(logger, sinks)
 *
 *   LoggerFactory::configureFromJsonFile(path)
 *     -> LoggerConfigurator::configureFromJsonFile(path)
 *          -> reads file -> configureFromJsonString(json)
 * @endcode
 */
class LoggerConfigurator
{
public:
    /**
     * @brief Apply configuration from an in-memory JSON string to all named loggers.
     *
     * Throws std::runtime_error if the JSON is invalid.
     *
     * @param json JSON configuration string.
     */
    static void configureFromJsonString(const std::string& json);

    /**
     * @brief Apply configuration from a JSON file to all named loggers.
     *
     * Reads the file at @p filePath then delegates to configureFromJsonString().
     * Throws std::runtime_error if the file is missing or the JSON is invalid.
     *
     * @param filePath Absolute or relative path to the JSON config file.
     */
    static void configureFromJsonFile(const std::string& filePath);

private:
    LoggerConfigurator() = delete;
    ~LoggerConfigurator() = delete;
};

}} // namespace sk::logger

#endif // SK_LOGGER_CONFIGURATOR_H

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
 * @brief Reads a JSON configuration file and applies it to the active backend.
 *
 * Call chain:
 * @code
 *   LoggerFactory::configure(path)
 *     -> LoggerConfigurator::configure(path)
 *          -> JsonConfigParser::parse(path)
 *          -> for each LoggerConfig:
 *               logger = LoggerFactoryImpl::getLogger(name)
 *               logger->setLevel(...)
 *               LoggerFactoryImpl::configureLogger(logger, sinks)
 *                 -> m_backend->configureLogger(logger, sinks)
 * @endcode
 */
class LoggerConfigurator
{
public:
    /**
     * @brief Apply configuration from a JSON file to all named loggers.
     *
     * Throws std::runtime_error if the file is missing or the JSON is invalid.
     *
     * @param filePath Absolute or relative path to the JSON config file.
     */
    static void configure(const std::string& filePath);

private:
    LoggerConfigurator() = delete;
    ~LoggerConfigurator() = delete;
};

}} // namespace sk::logger

#endif // SK_LOGGER_CONFIGURATOR_H

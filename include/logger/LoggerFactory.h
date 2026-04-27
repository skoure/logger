/**
 * @file LoggerFactory.h
 * @brief Singleton factory for creating and retrieving logger instances.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 * @date Last modified: November 08, 2025
 */
#ifndef SK_LOGGER_FACTORY_H
#define SK_LOGGER_FACTORY_H

#include <logger/Logger.h>
#include <logger/LevelNames.h>
#include <ostream>
#include <string>

namespace sk { namespace logger {

/**
 * @class LoggerFactory
 * @brief Singleton factory for creating and retrieving logger instances.
 *
 * This class manages logger instances by name and provides a unified
 * interface for obtaining loggers of the selected implementation. It
 * ensures thread-safe access and supports backend flexibility via the
 * logging facade.
 */
class LoggerFactory {
public:
    /**
     * @brief Get the singleton LoggerFactory instance.
     */
    static LoggerFactory& getInstance();
    ~LoggerFactory(void);

    /**
     * @brief Get or create a logger by name. Hierarchy-aware.
     * @param pName Logger name (e.g. "App.Database")
     * @return LoggerPtr instance
     */
    static LoggerPtr getLogger(const char* pName);

    /**
     * @brief Set the level name strings used by the %p pattern token.
     *
     * Replaces the six level name strings (FATAL, ERROR, WARN, INFO, DEBUG, TRACE)
     * for all three backends simultaneously. Call once at startup, before any
     * logging takes place. The strings pointed to by @p names must outlive all
     * subsequent logging calls.
     *
     * @param names Struct containing a name string for each of the six levels.
     */
    static void setLevelNames(const LevelNames& names);

    /**
     * @brief Apply a JSON configuration string to the active backend.
     *
     * Parses @p json and, for each named logger entry, sets its level and
     * delegates sink configuration to the active backend.
     *
     * Throws ParseException if the JSON is invalid.
     *
     * @param json JSON configuration string.
     */
    static void configureFromJsonString(const std::string& json);

    /**
     * @brief Apply a JSON configuration file to the active backend.
     *
     * Reads the file at @p filePath, then delegates to configureFromJsonString().
     *
     * Throws ParseException if the file is missing or contains invalid JSON.
     *
     * @param filePath Path to the JSON logging configuration file.
     */
    static void configureFromJsonFile(const std::string& filePath);

    /**
     * @brief Configure a logger to write to an arbitrary std::ostream.
     *
     * Translates @p canonicalPattern and attaches the backend's ostream sink
     * to the named logger (creating it if absent).
     *
     * Supported by all three backends (simple, spdlog, log4cxx).
     *
     * @warning The caller must ensure @p os outlives all loggers configured
     *          against it.
     *
     * @param name             Logger name (e.g. "App.Database").
     * @param os               Output stream to write to.
     * @param canonicalPattern Log4j-style pattern (e.g. "[%p] [%c] %m%n").
     */
    static void configureLoggerWithOstream(const char* name, std::ostream& os,
                                           const std::string& canonicalPattern);

};

}} // namespace

#endif // SK_LOGGER_FACTORY_H

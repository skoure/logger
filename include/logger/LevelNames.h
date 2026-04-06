/**
 * @file LevelNames.h
 * @brief Configurable level name strings for the %p pattern token.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: April 05, 2026
 */
#ifndef SK_LEVEL_NAMES_H
#define SK_LEVEL_NAMES_H

namespace sk { namespace logger {

/**
 * @struct LevelNames
 * @brief Holds the string label for each log level as rendered by the %p pattern token.
 *
 * The default values are uppercase abbreviations consistent across all three backends:
 * FATAL, ERROR, WARN, INFO, DEBUG, TRACE.
 *
 * Override any or all fields and pass the struct to LoggerFactory::setLevelNames()
 * before any logging takes place. The pointed-to strings must outlive all logging.
 *
 * @code
 * sk::logger::LevelNames names;
 * names.warn = "WARNING";
 * sk::logger::LoggerFactory::setLevelNames(names);
 * @endcode
 */
struct LevelNames {
    /** String used when formatting the %p token for Fatal level events. */
    const char* fatal = "FATAL";
    /** String used when formatting the %p token for Error level events. */
    const char* error = "ERROR";
    /** String used when formatting the %p token for Warn level events. */
    const char* warn  = "WARN";
    /** String used when formatting the %p token for Info level events. */
    const char* info  = "INFO";
    /** String used when formatting the %p token for Debug level events. */
    const char* debug = "DEBUG";
    /** String used when formatting the %p token for Trace level events. */
    const char* trace = "TRACE";
};

}} // namespace sk::logger

#endif // SK_LEVEL_NAMES_H

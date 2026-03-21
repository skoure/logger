/**
 * @file Logger.h
 * @brief Logging interface for all logger implementations.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 * @date Last modified: November 08, 2025
 */
#ifndef SK_LOGGER_H
#define SK_LOGGER_H

#include <exception>
#include <memory>
#include <string>

namespace sk { namespace logger {

/**
 * @class Logger
 * @brief Abstract base class for all logger implementations.
 *
 * Provides a unified interface for logging messages at various severity levels.
 * Concrete implementations should inherit from this class and implement all pure virtual methods.
 */
class Logger {
public:

    Logger() {};
    virtual ~Logger() {};

    /**
     * @enum Level
     * @brief Log severity levels.
     */
    enum class Level
    {
        Fatal = 0,
        Error,
        Warn,
        Info,
        Debug,
        Trace
    };

    /**
     * @brief Returns the name of the logger.
     * @return Logger name as a std::string.
     */
    virtual const std::string getName() = 0;

    /**
     * @brief Returns the current logging level for this logger.
     * @return Current log level.
     */
    virtual Level getLevel() = 0;

    /**
     * @brief Sets the logging level for this logger.
     * @param level New log level to set.
     */
    virtual void setLevel(Level level) = 0;

    /**
     * @brief Checks if FATAL level logging is enabled.
     * @return True if FATAL or higher is enabled, false otherwise.
     */
    virtual bool isFatalEnabled() const = 0;
    
    /**
     * @brief Checks if ERROR level logging is enabled.
     * @return True if ERROR or higher is enabled, false otherwise.
     */
    virtual bool isErrorEnabled() const = 0;

    /**
     * @brief Checks if WARN level logging is enabled.
     * @return True if WARN or higher is enabled, false otherwise.
     */
    virtual bool isWarnEnabled() const = 0;

    /**
     * @brief Checks if INFO level logging is enabled.
     * @return True if INFO or higher is enabled, false otherwise.
     */
    virtual bool isInfoEnabled() const = 0;

    /**
     * @brief Checks if DEBUG level logging is enabled.
     * @details Use this to avoid constructing debug messages when not enabled.
     * @return True if DEBUG or higher is enabled, false otherwise.
     */
    virtual bool isDebugEnabled() const = 0;

    /**
     * @brief Checks if TRACE level logging is enabled.
     * @return True if TRACE is enabled, false otherwise.
     */
    virtual bool isTraceEnabled() const = 0;

    /**
     * @brief Logs a formatted fatal message if FATAL level is enabled.
     * @param fmt Format string.
     * @param ... Arguments for format string.
     */
    virtual void fatal(const char *fmt, ...) = 0;

    /**
     * @brief Logs a formatted error message if ERROR level is enabled.
     * @param fmt Format string.
     * @param ... Arguments for format string.
     */
    virtual void error(const char *fmt, ...) = 0;
    /**
     * @brief Logs a formatted warning message if WARN level is enabled.
     * @param fmt Format string.
     * @param ... Arguments for format string.
     */
    virtual void warn(const char *fmt, ...) = 0;

    /**
     * @brief Logs a formatted info message if INFO level is enabled.
     * @param fmt Format string.
     * @param ... Arguments for format string.
     */
    virtual void info(const char *fmt, ...) = 0;

    /**
     * @brief Logs a formatted debug message if DEBUG level is enabled.
     * @param fmt Format string.
     * @param ... Arguments for format string.
     */
    virtual void debug(const char *fmt, ...) = 0;

    /**
     * @brief Logs a formatted trace message if TRACE level is enabled.
     * @param fmt Format string.
     * @param ... Arguments for format string.
     */
    virtual void trace(const char *fmt, ...) = 0;

    /**
     * @brief Logs an exception at ERROR level with a context message and stacktrace.
     *
     * The stacktrace is captured at the call site (catch block), not the throw site.
     *
     * @param msg Context message describing the operation that failed.
     * @param ex  The exception to log.
     */
    virtual void error(const char* msg, const std::exception& ex) = 0;

    /**
     * @brief Logs an exception at FATAL level with a context message and stacktrace.
     *
     * The stacktrace is captured at the call site (catch block), not the throw site.
     *
     * @param msg Context message describing the operation that failed.
     * @param ex  The exception to log.
     */
    virtual void fatal(const char* msg, const std::exception& ex) = 0;

protected:

};

/**
 * @typedef LoggerPtr
 * @brief Shared pointer type for Logger instances.
 */
typedef std::shared_ptr<Logger> LoggerPtr;

}} // namespace

#endif // SK_LOGGER_H

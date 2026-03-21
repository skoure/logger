/**
 * @file LoggerBase.h
 * @brief Concrete base class providing the format-and-dispatch pipeline for all backends.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#ifndef SK_LOGGER_BASE_H
#define SK_LOGGER_BASE_H

#include <logger/Logger.h>
#include <LogRecord.h>
#include <stdarg.h>

namespace sk { namespace logger {

/**
 * @class LoggerBase
 * @brief Implements all public Logger log methods in terms of a single append() hook.
 *
 * Subclasses (SimpleLogger, SpdlogLogger, Log4CxxLogger) only need to implement:
 *   - getName(), getLevel(), setLevel()
 *   - isXxxEnabled() — six level-check methods
 *   - append(const LogRecord&) — the backend write operation
 *
 * This base class handles va_list capture, dynamic message formatting (no fixed
 * buffer limit), LogRecord construction, and exception formatting — once, in one
 * place, for all backends.
 */
class LoggerBase : public Logger
{
public:
    // --- Concrete implementations of Logger's pure virtual log methods ---

    void fatal(const char* fmt, ...) override;
    void error(const char* fmt, ...) override;
    void warn (const char* fmt, ...) override;
    void info (const char* fmt, ...) override;
    void debug(const char* fmt, ...) override;
    void trace(const char* fmt, ...) override;

    void error(const char* msg, const std::exception& ex) override;
    void fatal(const char* msg, const std::exception& ex) override;

    /**
     * @brief Converts a Level value to its uppercase string name.
     * @param level The log level.
     * @return Pointer to a static string, e.g. "FATAL", "DEBUG".
     */
    static const char* levelToString(Level level);

protected:
    /**
     * @brief Delivers a fully-formed LogRecord to the backend.
     *
     * Called by logImpl() after the message has been formatted and the
     * LogRecord populated. Implementations should write or forward the
     * record without any further formatting of level/name/message.
     *
     * @param record The log record to append.
     */
    virtual void append(const LogRecord& record) = 0;

private:
    /**
     * @brief Formats the message, builds a LogRecord, and calls append().
     * @param level Severity level for this event.
     * @param fmt   printf-style format string.
     * @param args  va_list for the format arguments.
     */
    void logImpl(Level level, const char* fmt, va_list args);
};

}} // namespace sk::logger

#endif // SK_LOGGER_BASE_H

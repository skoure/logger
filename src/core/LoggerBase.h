/**
 * @file LoggerBase.h
 * @brief Concrete base class providing level management and the format-and-dispatch pipeline.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#ifndef SK_LOGGER_BASE_H
#define SK_LOGGER_BASE_H

#include <logger/Logger.h>
#include <LogRecord.h>
#include <memory>
#include <stdarg.h>

namespace sk { namespace logger {

/**
 * @class LoggerBase
 * @brief Implements level management and all public Logger log methods in terms of a single append() hook.
 *
 * Provides:
 *   - Hierarchical level inheritance: getLevel() walks up the parent chain if no explicit
 *     level has been set on this logger. setLevel() / clearLevel() manage the explicit marker.
 *   - Additivity flag.
 *   - All isXxxEnabled() checks via getLevel().
 *   - printf-style message formatting and LogRecord dispatch for all log methods.
 *
 * Subclasses only need to implement:
 *   - getName()
 *   - append(const LogRecord&) — the backend write operation
 *
 * Backends that manage level natively (e.g. Log4CxxLogger) override getLevel(),
 * setLevel(), clearLevel(), and isLevelExplicitlySet() to delegate to their own
 * internal state, and override onLevelChanged() is NOT needed for those.
 */
class LoggerBase : public Logger
{
public:
    // --- Level management (Logger overrides) ---

    Level getLevel() const override;
    void  setLevel(Level level) override;
    void  clearLevel() override;
    bool  isLevelExplicitlySet() const override;

    // --- Level checks (Logger overrides) ---

    bool isFatalEnabled() const override;
    bool isErrorEnabled() const override;
    bool isWarnEnabled()  const override;
    bool isInfoEnabled()  const override;
    bool isDebugEnabled() const override;
    bool isTraceEnabled() const override;

    // --- Concrete implementations of Logger's pure virtual log methods ---

    void fatal(const char* fmt, ...) override;
    void error(const char* fmt, ...) override;
    void warn (const char* fmt, ...) override;
    void info (const char* fmt, ...) override;
    void debug(const char* fmt, ...) override;
    void trace(const char* fmt, ...) override;

    void error(const char* msg, const std::exception& ex) override;
    void fatal(const char* msg, const std::exception& ex) override;

    // --- Marker overloads ---

    void fatal(const Marker& marker, const char* fmt, ...) override;
    void error(const Marker& marker, const char* fmt, ...) override;
    void warn (const Marker& marker, const char* fmt, ...) override;
    void info (const Marker& marker, const char* fmt, ...) override;
    void debug(const Marker& marker, const char* fmt, ...) override;
    void trace(const Marker& marker, const char* fmt, ...) override;

    void error(const Marker& marker, const char* msg,
               const std::exception& ex) override;
    void fatal(const Marker& marker, const char* msg,
               const std::exception& ex) override;

    /**
     * @brief Converts a Level value to its uppercase string name.
     * @param level The log level.
     * @return Pointer to a static string, e.g. "FATAL", "DEBUG".
     */
    static const char* levelToString(Level level);

    /**
     * @brief Sets the parent logger used for level inheritance.
     *
     * Called once by LoggerFactoryImpl when the logger is registered.
     * The parent is held as a weak_ptr to avoid ownership cycles.
     *
     * @param parent Weak reference to the parent logger.
     */
    void setParent(const std::weak_ptr<Logger>& parent);

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

    /**
     * @brief Called after setLevel() stores a new explicit level.
     *
     * Override in backends (e.g. SpdlogLogger) to sync their internal
     * level state. The default implementation is a no-op.
     *
     * @param level The newly set level.
     */
    virtual void onLevelChanged(Level /*level*/) {}

private:
    Level                 m_level              = Level::Info;
    bool                  m_levelExplicitlySet = false;
    std::weak_ptr<Logger> m_parent;

    /**
     * @brief Formats the message, builds a LogRecord, and calls append().
     * @param level Severity level for this event.
     * @param fmt   printf-style format string.
     * @param args  va_list for the format arguments.
     */
    void logImpl(Level level, const char* fmt, va_list args);

    /**
     * @brief Like logImpl() but also sets record.marker from @p marker.
     * @param level  Severity level for this event.
     * @param marker Marker to attach to the log record.
     * @param fmt    printf-style format string.
     * @param args   va_list for the format arguments.
     */
    void logImplWithMarker(Level level, const Marker& marker,
                           const char* fmt, va_list args);
};

}} // namespace sk::logger

#endif // SK_LOGGER_BASE_H

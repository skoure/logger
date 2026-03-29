/**
 * @file SimpleLoggerPattern.h
 * @brief Pattern renderer for the SimpleLogger backend.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 *
 * Expands canonical log4j-style pattern tokens using fields from a LogRecord.
 *
 * Supported tokens:
 *   %m  — message
 *   %p  — level name (FATAL, ERROR, …)
 *   %c  — logger name
 *   %t  — thread id (decimal)
 *   %T  — thread name
 *   %M  — marker name (empty string if no marker)
 *   %d{strftime-fmt} — timestamp formatted with strftime
 *   %n  — newline (\n)
 *   %%  — literal %
 *   other tokens pass through unchanged
 */
#ifndef SK_SIMPLE_LOGGER_PATTERN_H
#define SK_SIMPLE_LOGGER_PATTERN_H

#include <LogRecord.h>
#include <string>

namespace sk { namespace logger {

/**
 * @class SimpleLoggerPattern
 * @brief Expands a canonical pattern string using a LogRecord's fields.
 */
class SimpleLoggerPattern
{
public:
    /**
     * @brief Render a canonical pattern for the given log record.
     * @param pattern Canonical log4j-style pattern string.
     * @param record  Log record providing field values.
     * @return Fully expanded log line string.
     */
    static std::string render(const std::string& pattern,
                              const LogRecord& record);

private:
    SimpleLoggerPattern() = delete;
    ~SimpleLoggerPattern() = delete;
};

}} // namespace sk::logger

#endif // SK_SIMPLE_LOGGER_PATTERN_H

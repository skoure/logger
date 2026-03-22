/**
 * @file LoggerBase.cpp
 * @brief Format-and-dispatch pipeline shared by all logger backends.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#include <LoggerBase.h>
#include <LoggerUtils.h>
#include <chrono>
#include <cstdio>
#include <stdarg.h>

using namespace sk::logger;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/**
 * Formats a printf-style message into a std::string without a fixed buffer.
 * Uses vsnprintf twice: first to measure the required length, then to fill.
 */
static std::string formatMessage(const char* fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);
    int size = std::vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);

    if (size <= 0)
        return {};

    std::string result(static_cast<std::size_t>(size) + 1, '\0');
    std::vsnprintf(&result[0], result.size(), fmt, args);
    result.resize(static_cast<std::size_t>(size));
    return result;
}

// ---------------------------------------------------------------------------
// LoggerBase — level / additivity management
// ---------------------------------------------------------------------------

Logger::Level LoggerBase::getLevel() const
{
    if (m_levelExplicitlySet)
        return m_level;
    auto parent = m_parent.lock();
    return parent ? parent->getLevel() : Level::Info;
}

void LoggerBase::setLevel(Level level)
{
    m_level              = level;
    m_levelExplicitlySet = true;
    onLevelChanged(level);
}

void LoggerBase::clearLevel()
{
    m_levelExplicitlySet = false;
}

bool LoggerBase::isLevelExplicitlySet() const
{
    return m_levelExplicitlySet;
}


void LoggerBase::setParent(const std::weak_ptr<Logger>& parent)
{
    m_parent = parent;
}

bool LoggerBase::isFatalEnabled() const { return getLevel() >= Level::Fatal; }
bool LoggerBase::isErrorEnabled() const { return getLevel() >= Level::Error; }
bool LoggerBase::isWarnEnabled()  const { return getLevel() >= Level::Warn;  }
bool LoggerBase::isInfoEnabled()  const { return getLevel() >= Level::Info;  }
bool LoggerBase::isDebugEnabled() const { return getLevel() >= Level::Debug; }
bool LoggerBase::isTraceEnabled() const { return getLevel() >= Level::Trace; }

// ---------------------------------------------------------------------------
// LoggerBase — static helpers
// ---------------------------------------------------------------------------

const char* LoggerBase::levelToString(Level level)
{
    switch (level)
    {
    case Level::Fatal: return "FATAL";
    case Level::Error: return "ERROR";
    case Level::Warn:  return "WARN";
    case Level::Info:  return "INFO";
    case Level::Debug: return "DEBUG";
    case Level::Trace: return "TRACE";
    default:           return "UNKNOWN";
    }
}

void LoggerBase::logImpl(Level level, const char* fmt, va_list args)
{
    LogRecord record;
    record.level      = level;
    record.loggerName = getName();
    record.message    = formatMessage(fmt, args);
    record.timestamp  = std::chrono::system_clock::now();
    append(record);
}

void LoggerBase::fatal(const char* fmt, ...)
{
    if (isFatalEnabled())
    {
        va_list args;
        va_start(args, fmt);
        logImpl(Level::Fatal, fmt, args);
        va_end(args);
    }
}

void LoggerBase::error(const char* fmt, ...)
{
    if (isErrorEnabled())
    {
        va_list args;
        va_start(args, fmt);
        logImpl(Level::Error, fmt, args);
        va_end(args);
    }
}

void LoggerBase::warn(const char* fmt, ...)
{
    if (isWarnEnabled())
    {
        va_list args;
        va_start(args, fmt);
        logImpl(Level::Warn, fmt, args);
        va_end(args);
    }
}

void LoggerBase::info(const char* fmt, ...)
{
    if (isInfoEnabled())
    {
        va_list args;
        va_start(args, fmt);
        logImpl(Level::Info, fmt, args);
        va_end(args);
    }
}

void LoggerBase::debug(const char* fmt, ...)
{
    if (isDebugEnabled())
    {
        va_list args;
        va_start(args, fmt);
        logImpl(Level::Debug, fmt, args);
        va_end(args);
    }
}

void LoggerBase::trace(const char* fmt, ...)
{
    if (isTraceEnabled())
    {
        va_list args;
        va_start(args, fmt);
        logImpl(Level::Trace, fmt, args);
        va_end(args);
    }
}

void LoggerBase::error(const char* msg, const std::exception& ex)
{
    if (isErrorEnabled())
    {
        LogRecord record;
        record.level      = Level::Error;
        record.loggerName = getName();
        record.message    = formatException(msg, ex);
        record.timestamp  = std::chrono::system_clock::now();
        append(record);
    }
}

void LoggerBase::fatal(const char* msg, const std::exception& ex)
{
    if (isFatalEnabled())
    {
        LogRecord record;
        record.level      = Level::Fatal;
        record.loggerName = getName();
        record.message    = formatException(msg, ex);
        record.timestamp  = std::chrono::system_clock::now();
        append(record);
    }
}

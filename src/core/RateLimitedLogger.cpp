/**
 * @file RateLimitedLogger.cpp
 * @brief Implementation of RateLimitedLogger.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 22, 2026
 */
#include "RateLimitedLoggerImpl.h"
#include <cstdarg>

namespace sk { namespace logger {

// ---------------------------------------------------------------------------
// RateLimitedLogger constructors & lifecycle
// ---------------------------------------------------------------------------

RateLimitedLogger::RateLimitedLogger(std::unique_ptr<Impl> impl)
    : m_impl(std::move(impl))
{
}

RateLimitedLogger::~RateLimitedLogger() = default;

RateLimitedLogger::RateLimitedLogger(RateLimitedLogger&& other) noexcept = default;

RateLimitedLogger& RateLimitedLogger::operator=(RateLimitedLogger&& other) noexcept = default;

LoggerPtr RateLimitedLogger::getLogger() const
{
    return m_impl ? m_impl->getLogger() : nullptr;
}

// ---------------------------------------------------------------------------
// Non-logging methods (transparent forwarding)
// ---------------------------------------------------------------------------

std::string RateLimitedLogger::getName() const
{
    auto logger = getLogger();
    return logger ? logger->getName() : std::string{};
}

Logger::Level RateLimitedLogger::getLevel() const
{
    auto logger = getLogger();
    return logger ? logger->getLevel() : Logger::Level::Info;
}

void RateLimitedLogger::setLevel(Level level)
{
    if (auto logger = getLogger())
        logger->setLevel(level);
}

void RateLimitedLogger::clearLevel()
{
    if (auto logger = getLogger())
        logger->clearLevel();
}

bool RateLimitedLogger::isLevelExplicitlySet() const
{
    auto logger = getLogger();
    return logger ? logger->isLevelExplicitlySet() : false;
}

void RateLimitedLogger::setFlushOn(Level level)
{
    if (auto logger = getLogger())
        logger->setFlushOn(level);
}

void RateLimitedLogger::clearFlushOn()
{
    if (auto logger = getLogger())
        logger->clearFlushOn();
}

std::optional<Logger::Level> RateLimitedLogger::getFlushOn() const
{
    auto logger = getLogger();
    return logger ? logger->getFlushOn() : std::nullopt;
}

bool RateLimitedLogger::isFatalEnabled() const
{
    auto logger = getLogger();
    return logger ? logger->isFatalEnabled() : false;
}

bool RateLimitedLogger::isErrorEnabled() const
{
    auto logger = getLogger();
    return logger ? logger->isErrorEnabled() : false;
}

bool RateLimitedLogger::isWarnEnabled() const
{
    auto logger = getLogger();
    return logger ? logger->isWarnEnabled() : false;
}

bool RateLimitedLogger::isInfoEnabled() const
{
    auto logger = getLogger();
    return logger ? logger->isInfoEnabled() : false;
}

bool RateLimitedLogger::isDebugEnabled() const
{
    auto logger = getLogger();
    return logger ? logger->isDebugEnabled() : false;
}

bool RateLimitedLogger::isTraceEnabled() const
{
    auto logger = getLogger();
    return logger ? logger->isTraceEnabled() : false;
}

bool RateLimitedLogger::isLevelActive(Level level) const
{
    switch (level)
    {
    case Level::Fatal: return isFatalEnabled();
    case Level::Error: return isErrorEnabled();
    case Level::Warn:  return isWarnEnabled();
    case Level::Info:  return isInfoEnabled();
    case Level::Debug: return isDebugEnabled();
    case Level::Trace: return isTraceEnabled();
    default:           return false;
    }
}

// ---------------------------------------------------------------------------
// Private core dispatch helper
// ---------------------------------------------------------------------------

void RateLimitedLogger::logFormattedCore(Level level,
                                         const Marker* marker,
                                         const MarkerList* markers,
                                         const char* fmt,
                                         va_list args)
{
    if (!m_impl)
        return;
    int suppressed = 0;
    if (!m_impl->checkAdmission(suppressed))
        return;
    std::string msg = m_impl->formatAndSubstitute(fmt, args, suppressed);
    auto logger = m_impl->getLogger();
    if (!logger)
        return;
    if (markers)
    {
        switch (level)
        {
        case Level::Fatal: logger->fatal(*markers, "%s", msg.c_str()); break;
        case Level::Error: logger->error(*markers, "%s", msg.c_str()); break;
        case Level::Warn:  logger->warn (*markers, "%s", msg.c_str()); break;
        case Level::Info:  logger->info (*markers, "%s", msg.c_str()); break;
        case Level::Debug: logger->debug(*markers, "%s", msg.c_str()); break;
        case Level::Trace: logger->trace(*markers, "%s", msg.c_str()); break;
        }
    }
    else if (marker)
    {
        switch (level)
        {
        case Level::Fatal: logger->fatal(*marker, "%s", msg.c_str()); break;
        case Level::Error: logger->error(*marker, "%s", msg.c_str()); break;
        case Level::Warn:  logger->warn (*marker, "%s", msg.c_str()); break;
        case Level::Info:  logger->info (*marker, "%s", msg.c_str()); break;
        case Level::Debug: logger->debug(*marker, "%s", msg.c_str()); break;
        case Level::Trace: logger->trace(*marker, "%s", msg.c_str()); break;
        }
    }
    else
    {
        switch (level)
        {
        case Level::Fatal: logger->fatal("%s", msg.c_str()); break;
        case Level::Error: logger->error("%s", msg.c_str()); break;
        case Level::Warn:  logger->warn ("%s", msg.c_str()); break;
        case Level::Info:  logger->info ("%s", msg.c_str()); break;
        case Level::Debug: logger->debug("%s", msg.c_str()); break;
        case Level::Trace: logger->trace("%s", msg.c_str()); break;
        }
    }
}

// ---------------------------------------------------------------------------
// Standard printf-style logging methods — plain
// ---------------------------------------------------------------------------

void RateLimitedLogger::fatal(const char* fmt, ...)
{
    if (!m_impl || !isFatalEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Fatal, nullptr, nullptr, fmt, args);
    va_end(args);
}

void RateLimitedLogger::error(const char* fmt, ...)
{
    if (!m_impl || !isErrorEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Error, nullptr, nullptr, fmt, args);
    va_end(args);
}

void RateLimitedLogger::warn(const char* fmt, ...)
{
    if (!m_impl || !isWarnEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Warn, nullptr, nullptr, fmt, args);
    va_end(args);
}

void RateLimitedLogger::info(const char* fmt, ...)
{
    if (!m_impl || !isInfoEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Info, nullptr, nullptr, fmt, args);
    va_end(args);
}

void RateLimitedLogger::debug(const char* fmt, ...)
{
    if (!m_impl || !isDebugEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Debug, nullptr, nullptr, fmt, args);
    va_end(args);
}

void RateLimitedLogger::trace(const char* fmt, ...)
{
    if (!m_impl || !isTraceEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Trace, nullptr, nullptr, fmt, args);
    va_end(args);
}

// ---------------------------------------------------------------------------
// Marker overloads
// ---------------------------------------------------------------------------

void RateLimitedLogger::fatal(const Marker& marker, const char* fmt, ...)
{
    if (!m_impl || !isFatalEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Fatal, &marker, nullptr, fmt, args);
    va_end(args);
}

void RateLimitedLogger::error(const Marker& marker, const char* fmt, ...)
{
    if (!m_impl || !isErrorEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Error, &marker, nullptr, fmt, args);
    va_end(args);
}

void RateLimitedLogger::warn(const Marker& marker, const char* fmt, ...)
{
    if (!m_impl || !isWarnEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Warn, &marker, nullptr, fmt, args);
    va_end(args);
}

void RateLimitedLogger::info(const Marker& marker, const char* fmt, ...)
{
    if (!m_impl || !isInfoEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Info, &marker, nullptr, fmt, args);
    va_end(args);
}

void RateLimitedLogger::debug(const Marker& marker, const char* fmt, ...)
{
    if (!m_impl || !isDebugEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Debug, &marker, nullptr, fmt, args);
    va_end(args);
}

void RateLimitedLogger::trace(const Marker& marker, const char* fmt, ...)
{
    if (!m_impl || !isTraceEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Trace, &marker, nullptr, fmt, args);
    va_end(args);
}

// ---------------------------------------------------------------------------
// MarkerList overloads
// ---------------------------------------------------------------------------

void RateLimitedLogger::fatal(const MarkerList& markers, const char* fmt, ...)
{
    if (!m_impl || !isFatalEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Fatal, nullptr, &markers, fmt, args);
    va_end(args);
}

void RateLimitedLogger::error(const MarkerList& markers, const char* fmt, ...)
{
    if (!m_impl || !isErrorEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Error, nullptr, &markers, fmt, args);
    va_end(args);
}

void RateLimitedLogger::warn(const MarkerList& markers, const char* fmt, ...)
{
    if (!m_impl || !isWarnEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Warn, nullptr, &markers, fmt, args);
    va_end(args);
}

void RateLimitedLogger::info(const MarkerList& markers, const char* fmt, ...)
{
    if (!m_impl || !isInfoEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Info, nullptr, &markers, fmt, args);
    va_end(args);
}

void RateLimitedLogger::debug(const MarkerList& markers, const char* fmt, ...)
{
    if (!m_impl || !isDebugEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Debug, nullptr, &markers, fmt, args);
    va_end(args);
}

void RateLimitedLogger::trace(const MarkerList& markers, const char* fmt, ...)
{
    if (!m_impl || !isTraceEnabled()) return;
    va_list args; va_start(args, fmt);
    logFormattedCore(Level::Trace, nullptr, &markers, fmt, args);
    va_end(args);
}

// ---------------------------------------------------------------------------
// Exception logging methods
// ---------------------------------------------------------------------------

void RateLimitedLogger::error(const char* msg, const std::exception& ex)
{
    if (!m_impl || !isErrorEnabled()) return;
    int suppressed = 0;
    if (!m_impl->checkAdmission(suppressed)) return;
    std::string context = m_impl->substituteSuppressed(msg, suppressed);
    if (auto logger = m_impl->getLogger())
        logger->error(context.c_str(), ex);
}

void RateLimitedLogger::fatal(const char* msg, const std::exception& ex)
{
    if (!m_impl || !isFatalEnabled()) return;
    int suppressed = 0;
    if (!m_impl->checkAdmission(suppressed)) return;
    std::string context = m_impl->substituteSuppressed(msg, suppressed);
    if (auto logger = m_impl->getLogger())
        logger->fatal(context.c_str(), ex);
}

void RateLimitedLogger::error(const Marker& marker, const char* msg, const std::exception& ex)
{
    if (!m_impl || !isErrorEnabled()) return;
    int suppressed = 0;
    if (!m_impl->checkAdmission(suppressed)) return;
    std::string context = m_impl->substituteSuppressed(msg, suppressed);
    if (auto logger = m_impl->getLogger())
        logger->error(marker, context.c_str(), ex);
}

void RateLimitedLogger::fatal(const Marker& marker, const char* msg, const std::exception& ex)
{
    if (!m_impl || !isFatalEnabled()) return;
    int suppressed = 0;
    if (!m_impl->checkAdmission(suppressed)) return;
    std::string context = m_impl->substituteSuppressed(msg, suppressed);
    if (auto logger = m_impl->getLogger())
        logger->fatal(marker, context.c_str(), ex);
}

void RateLimitedLogger::error(const MarkerList& markers, const char* msg, const std::exception& ex)
{
    if (!m_impl || !isErrorEnabled()) return;
    int suppressed = 0;
    if (!m_impl->checkAdmission(suppressed)) return;
    std::string context = m_impl->substituteSuppressed(msg, suppressed);
    if (auto logger = m_impl->getLogger())
        logger->error(markers, context.c_str(), ex);
}

void RateLimitedLogger::fatal(const MarkerList& markers, const char* msg, const std::exception& ex)
{
    if (!m_impl || !isFatalEnabled()) return;
    int suppressed = 0;
    if (!m_impl->checkAdmission(suppressed)) return;
    std::string context = m_impl->substituteSuppressed(msg, suppressed);
    if (auto logger = m_impl->getLogger())
        logger->fatal(markers, context.c_str(), ex);
}

// ---------------------------------------------------------------------------
// Lazy evaluation pipeline
// ---------------------------------------------------------------------------

void RateLimitedLogger::logLazyImpl(Level level, const Marker* marker, const MarkerList* markerList,
                                    const std::function<std::string(int)>& generator)
{
    if (!m_impl || !isLevelActive(level))
        return;

    int suppressed = 0;
    if (!m_impl->checkAdmission(suppressed))
        return;

    // Generator is ONLY invoked when admitted!
    std::string msg = generator(suppressed);
    std::string finalMsg = m_impl->substituteSuppressed(msg, suppressed);

    auto logger = m_impl->getLogger();
    if (!logger)
        return;

    if (markerList)
    {
        switch (level)
        {
        case Level::Fatal: logger->fatal(*markerList, "%s", finalMsg.c_str()); break;
        case Level::Error: logger->error(*markerList, "%s", finalMsg.c_str()); break;
        case Level::Warn:  logger->warn (*markerList, "%s", finalMsg.c_str()); break;
        case Level::Info:  logger->info (*markerList, "%s", finalMsg.c_str()); break;
        case Level::Debug: logger->debug(*markerList, "%s", finalMsg.c_str()); break;
        case Level::Trace: logger->trace(*markerList, "%s", finalMsg.c_str()); break;
        }
    }
    else if (marker)
    {
        switch (level)
        {
        case Level::Fatal: logger->fatal(*marker, "%s", finalMsg.c_str()); break;
        case Level::Error: logger->error(*marker, "%s", finalMsg.c_str()); break;
        case Level::Warn:  logger->warn (*marker, "%s", finalMsg.c_str()); break;
        case Level::Info:  logger->info (*marker, "%s", finalMsg.c_str()); break;
        case Level::Debug: logger->debug(*marker, "%s", finalMsg.c_str()); break;
        case Level::Trace: logger->trace(*marker, "%s", finalMsg.c_str()); break;
        }
    }
    else
    {
        switch (level)
        {
        case Level::Fatal: logger->fatal("%s", finalMsg.c_str()); break;
        case Level::Error: logger->error("%s", finalMsg.c_str()); break;
        case Level::Warn:  logger->warn ("%s", finalMsg.c_str()); break;
        case Level::Info:  logger->info ("%s", finalMsg.c_str()); break;
        case Level::Debug: logger->debug("%s", finalMsg.c_str()); break;
        case Level::Trace: logger->trace("%s", finalMsg.c_str()); break;
        }
    }
}

void RateLimitedLogger::logLazyExceptionImpl(Level level, const Marker* marker, const MarkerList* markerList,
                                             const std::function<std::string(int)>& generator,
                                             const std::exception& ex)
{
    if (!m_impl || !isLevelActive(level))
        return;

    int suppressed = 0;
    if (!m_impl->checkAdmission(suppressed))
        return;

    // Generator is ONLY invoked when admitted!
    std::string msg = generator(suppressed);
    std::string finalMsg = m_impl->substituteSuppressed(msg, suppressed);

    auto logger = m_impl->getLogger();
    if (!logger)
        return;

    if (markerList)
    {
        if (level == Level::Fatal)
            logger->fatal(*markerList, finalMsg.c_str(), ex);
        else
            logger->error(*markerList, finalMsg.c_str(), ex);
    }
    else if (marker)
    {
        if (level == Level::Fatal)
            logger->fatal(*marker, finalMsg.c_str(), ex);
        else
            logger->error(*marker, finalMsg.c_str(), ex);
    }
    else
    {
        if (level == Level::Fatal)
            logger->fatal(finalMsg.c_str(), ex);
        else
            logger->error(finalMsg.c_str(), ex);
    }
}

}} // namespace sk::logger

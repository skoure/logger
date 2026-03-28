/**
 * @file Log4CxxLogger.cpp
 * @brief Logger implementation that delegates logging to Apache Log4cxx.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 */
#include <Log4CxxLogger.h>
#include <log4cxx/mdc.h>

using namespace sk::logger;

Log4CxxLogger::Log4CxxLogger(std::string name)
{
    m_name    = std::move(name);
    m_pLogger = log4cxx::Logger::getLogger(m_name);
}

Log4CxxLogger::~Log4CxxLogger()
{
}

Logger::Level Log4CxxLogger::getLevel() const
{
    // Use getEffectiveLevel() so we always return a valid level, walking
    // up log4cxx's native hierarchy if this logger has no explicit level.
    log4cxx::LevelPtr levelPtr = m_pLogger->getEffectiveLevel();
    if      (levelPtr == log4cxx::Level::getFatal()) return Level::Fatal;
    else if (levelPtr == log4cxx::Level::getError()) return Level::Error;
    else if (levelPtr == log4cxx::Level::getWarn())  return Level::Warn;
    else if (levelPtr == log4cxx::Level::getInfo())  return Level::Info;
    else if (levelPtr == log4cxx::Level::getDebug()) return Level::Debug;
    else if (levelPtr == log4cxx::Level::getTrace()) return Level::Trace;
    return Level::Info;
}

void Log4CxxLogger::setLevel(Level level)
{
    switch (level)
    {
    case Level::Fatal: m_pLogger->setLevel(log4cxx::Level::getFatal()); break;
    case Level::Error: m_pLogger->setLevel(log4cxx::Level::getError()); break;
    case Level::Warn:  m_pLogger->setLevel(log4cxx::Level::getWarn());  break;
    case Level::Info:  m_pLogger->setLevel(log4cxx::Level::getInfo());  break;
    case Level::Debug: m_pLogger->setLevel(log4cxx::Level::getDebug()); break;
    case Level::Trace: m_pLogger->setLevel(log4cxx::Level::getTrace()); break;
    }
}

void Log4CxxLogger::clearLevel()
{
    // Setting to null reverts to log4cxx's native hierarchy inheritance.
    m_pLogger->setLevel(nullptr);
}

bool Log4CxxLogger::isLevelExplicitlySet() const
{
    // A null level pointer means the logger is inheriting from its parent.
    return m_pLogger->getLevel() != nullptr;
}

void Log4CxxLogger::append(const LogRecord& record)
{
    // Populate the log4cxx MDC with the marker name so that %X{marker}
    // in a PatternLayout can render it.
    const bool hasMarker = (record.marker != nullptr);
    if (hasMarker)
        log4cxx::MDC::put("marker", record.marker->getName());

    switch (record.level)
    {
    case Level::Fatal: LOG4CXX_FATAL(m_pLogger, record.message); break;
    case Level::Error: LOG4CXX_ERROR(m_pLogger, record.message); break;
    case Level::Warn:  LOG4CXX_WARN (m_pLogger, record.message); break;
    case Level::Info:  LOG4CXX_INFO (m_pLogger, record.message); break;
    case Level::Debug: LOG4CXX_DEBUG(m_pLogger, record.message); break;
    case Level::Trace: LOG4CXX_TRACE(m_pLogger, record.message); break;
    }

    if (hasMarker)
        log4cxx::MDC::remove("marker");
}

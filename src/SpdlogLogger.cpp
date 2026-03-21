/**
 * @file SpdlogLogger.cpp
 * @brief Logger implementation that delegates logging to spdlog.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 15, 2025
 * @date Last modified: March 21, 2026
 */
#include <SpdlogLogger.h>

#ifdef USE_SPDLOG

#include <spdlog/sinks/stdout_color_sinks.h>

using namespace sk::logger;

SpdlogLogger::SpdlogLogger(std::string name)
    : m_name(std::move(name)), m_level(Level::Info)
{
    m_pLogger = spdlog::get(m_name);
    if (!m_pLogger)
    {
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        m_pLogger = std::make_shared<spdlog::logger>(m_name, sink);
        m_pLogger->set_level(spdlog::level::info);
        spdlog::register_logger(m_pLogger);
    }
}

SpdlogLogger::~SpdlogLogger()
{
}

spdlog::level::level_enum SpdlogLogger::toSpdlogLevel(Level level)
{
    switch (level)
    {
    case Level::Fatal: return spdlog::level::critical;
    case Level::Error: return spdlog::level::err;
    case Level::Warn:  return spdlog::level::warn;
    case Level::Info:  return spdlog::level::info;
    case Level::Debug: return spdlog::level::debug;
    case Level::Trace: return spdlog::level::trace;
    default:           return spdlog::level::info;
    }
}

Logger::Level SpdlogLogger::fromSpdlogLevel(spdlog::level::level_enum level)
{
    switch (level)
    {
    case spdlog::level::critical: return Level::Fatal;
    case spdlog::level::err:      return Level::Error;
    case spdlog::level::warn:     return Level::Warn;
    case spdlog::level::info:     return Level::Info;
    case spdlog::level::debug:    return Level::Debug;
    case spdlog::level::trace:    return Level::Trace;
    default:                      return Level::Info;
    }
}

Logger::Level SpdlogLogger::getLevel()
{
    m_level = fromSpdlogLevel(m_pLogger->level());
    return m_level;
}

void SpdlogLogger::setLevel(Level level)
{
    m_level = level;
    m_pLogger->set_level(toSpdlogLevel(level));
}

bool SpdlogLogger::isFatalEnabled() const { return m_pLogger->should_log(spdlog::level::critical); }
bool SpdlogLogger::isErrorEnabled() const { return m_pLogger->should_log(spdlog::level::err);      }
bool SpdlogLogger::isWarnEnabled()  const { return m_pLogger->should_log(spdlog::level::warn);     }
bool SpdlogLogger::isInfoEnabled()  const { return m_pLogger->should_log(spdlog::level::info);     }
bool SpdlogLogger::isDebugEnabled() const { return m_pLogger->should_log(spdlog::level::debug);    }
bool SpdlogLogger::isTraceEnabled() const { return m_pLogger->should_log(spdlog::level::trace);    }

void SpdlogLogger::append(const LogRecord& record)
{
    switch (record.level)
    {
    case Level::Fatal: m_pLogger->critical(record.message); break;
    case Level::Error: m_pLogger->error   (record.message); break;
    case Level::Warn:  m_pLogger->warn    (record.message); break;
    case Level::Info:  m_pLogger->info    (record.message); break;
    case Level::Debug: m_pLogger->debug   (record.message); break;
    case Level::Trace: m_pLogger->trace   (record.message); break;
    }
}

#endif

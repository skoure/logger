/**
 * @file SpdlogLogger.cpp
 * @brief Logger implementation that delegates logging to spdlog.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 15, 2025
 */
#include <SpdlogLogger.h>
#include <SpdlogThreadLocal.h>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace sk::logger;

SpdlogLogger::SpdlogLogger(std::string name)
    : m_name(std::move(name))
{
    m_pLogger = spdlog::get(m_name);
    if (!m_pLogger)
    {
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        m_pLogger = std::make_shared<spdlog::logger>(m_name, sink);
        // sk::logger manages the hierarchy, so it owns all level and flush_on
        // decisions. LoggerBase::isXxxEnabled() gates every log call before
        // append() is reached, making spdlog's internal level filtering
        // redundant. Setting trace here lets everything through so that
        // LoggerBase remains the sole authority — no duplicate state to sync.
        m_pLogger->set_level(spdlog::level::trace);
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

void SpdlogLogger::append(const LogRecord& record)
{
    // Populate thread-local bridge so custom formatters can read LogRecord fields.
    spdlog_tls::markerName = record.marker ? record.marker->getName().c_str() : nullptr;
    spdlog_tls::threadName = record.threadName;

    // sk::logger manages the hierarchy and owns all level and flush_on decisions:
    //   - The constructor sets spdlog's internal level to trace so spdlog never
    //     filters; LoggerBase::isXxxEnabled() is the sole level gate.
    //   - Flush threshold is not delegated to spdlog's flush_on() either; instead,
    //     getFlushOn() walks the parent chain so child loggers inherit the threshold
    //     without needing it set individually on each one.
    switch (record.level)
    {
    case Level::Fatal: m_pLogger->critical(record.message); break;
    case Level::Error: m_pLogger->error   (record.message); break;
    case Level::Warn:  m_pLogger->warn    (record.message); break;
    case Level::Info:  m_pLogger->info    (record.message); break;
    case Level::Debug: m_pLogger->debug   (record.message); break;
    case Level::Trace: m_pLogger->trace   (record.message); break;
    }

    auto flushOn = getFlushOn();
    if (flushOn.has_value() && record.level <= *flushOn)
        m_pLogger->flush();

    // Clear after call to avoid stale data leaking to other threads.
    spdlog_tls::markerName = nullptr;
    spdlog_tls::threadName.clear();
}

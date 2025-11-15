/**
 * @file SpdlogLogger.cpp
 * @brief Logger implementation that delegates logging to spdlog.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 15, 2025
 * @date Last modified: November 15, 2025
 */
#include <SpdlogLogger.h>

#ifdef USE_SPDLOG

#include <spdlog/sinks/stdout_color_sinks.h>
#include <stdarg.h>
#include <cstdio>

#define LOG_MAX_BUF 4096

using namespace sk::logger;

SpdlogLogger::SpdlogLogger(std::string name) : m_name(std::move(name)), m_level(Level::Info)
{
	// Try to get existing logger, or create a new one if it doesn't exist
	m_pLogger = spdlog::get(m_name);
	if (!m_pLogger) {
		auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		m_pLogger = std::make_shared<spdlog::logger>(m_name, sink);
		m_pLogger->set_level(spdlog::level::info);
		spdlog::register_logger(m_pLogger);
	}
}

SpdlogLogger::~SpdlogLogger(void)
{
	// Don't unregister the logger - it may be used elsewhere
}

spdlog::level::level_enum SpdlogLogger::toSpdlogLevel(sk::logger::Logger::Level level)
{
	switch (level)
	{
	case sk::logger::Logger::Level::Fatal:
		return spdlog::level::critical;
	case sk::logger::Logger::Level::Error:
		return spdlog::level::err;
	case sk::logger::Logger::Level::Warn:
		return spdlog::level::warn;
	case sk::logger::Logger::Level::Info:
		return spdlog::level::info;
	case sk::logger::Logger::Level::Debug:
		return spdlog::level::debug;
	case sk::logger::Logger::Level::Trace:
		return spdlog::level::trace;
	default:
		return spdlog::level::info;
	}
}

sk::logger::Logger::Level SpdlogLogger::fromSpdlogLevel(spdlog::level::level_enum level)
{
	switch (level)
	{
	case spdlog::level::critical:
		return sk::logger::Logger::Level::Fatal;
	case spdlog::level::err:
		return sk::logger::Logger::Level::Error;
	case spdlog::level::warn:
		return sk::logger::Logger::Level::Warn;
	case spdlog::level::info:
		return sk::logger::Logger::Level::Info;
	case spdlog::level::debug:
		return sk::logger::Logger::Level::Debug;
	case spdlog::level::trace:
		return sk::logger::Logger::Level::Trace;
	default:
		return sk::logger::Logger::Level::Info;
	}
}

sk::logger::Logger::Level SpdlogLogger::getLevel()
{
	m_level = fromSpdlogLevel(m_pLogger->level());
	return m_level;
}

void SpdlogLogger::setLevel(sk::logger::Logger::Level level)
{
	m_level = level;
	m_pLogger->set_level(toSpdlogLevel(level));
}

bool SpdlogLogger::isFatalEnabled() const
{
	return m_pLogger->should_log(spdlog::level::critical);
}

bool SpdlogLogger::isErrorEnabled() const
{
	return m_pLogger->should_log(spdlog::level::err);
}

bool SpdlogLogger::isWarnEnabled() const
{
	return m_pLogger->should_log(spdlog::level::warn);
}

bool SpdlogLogger::isInfoEnabled() const
{
	return m_pLogger->should_log(spdlog::level::info);
}

bool SpdlogLogger::isDebugEnabled() const
{
	return m_pLogger->should_log(spdlog::level::debug);
}

bool SpdlogLogger::isTraceEnabled() const
{
	return m_pLogger->should_log(spdlog::level::trace);
}

void SpdlogLogger::fatal(const char *fmt, ...)
{
	if (isFatalEnabled())
	{
		char buf[LOG_MAX_BUF + 1] = {0};
		va_list argp;
		va_start(argp, fmt);
		std::vsnprintf(buf, LOG_MAX_BUF, fmt, argp);
		va_end(argp);
		m_pLogger->critical(buf);
	}
}

void SpdlogLogger::error(const char *fmt, ...)
{
	if (isErrorEnabled())
	{
		char buf[LOG_MAX_BUF + 1] = {0};
		va_list argp;
		va_start(argp, fmt);
		std::vsnprintf(buf, LOG_MAX_BUF, fmt, argp);
		va_end(argp);
		m_pLogger->error(buf);
	}
}

void SpdlogLogger::warn(const char *fmt, ...)
{
	if (isWarnEnabled())
	{
		char buf[LOG_MAX_BUF + 1] = {0};
		va_list argp;
		va_start(argp, fmt);
		std::vsnprintf(buf, LOG_MAX_BUF, fmt, argp);
		va_end(argp);
		m_pLogger->warn(buf);
	}
}

void SpdlogLogger::info(const char *fmt, ...)
{
	if (isInfoEnabled())
	{
		char buf[LOG_MAX_BUF + 1] = {0};
		va_list argp;
		va_start(argp, fmt);
		std::vsnprintf(buf, LOG_MAX_BUF, fmt, argp);
		va_end(argp);
		m_pLogger->info(buf);
	}
}

void SpdlogLogger::debug(const char *fmt, ...)
{
	if (isDebugEnabled())
	{
		char buf[LOG_MAX_BUF + 1] = {0};
		va_list argp;
		va_start(argp, fmt);
		std::vsnprintf(buf, LOG_MAX_BUF, fmt, argp);
		va_end(argp);
		m_pLogger->debug(buf);
	}
}

void SpdlogLogger::trace(const char *fmt, ...)
{
	if (isTraceEnabled())
	{
		char buf[LOG_MAX_BUF + 1] = {0};
		va_list argp;
		va_start(argp, fmt);
		std::vsnprintf(buf, LOG_MAX_BUF, fmt, argp);
		va_end(argp);
		m_pLogger->trace(buf);
	}
}

#endif

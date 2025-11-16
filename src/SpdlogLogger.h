/**
 * @file SpdlogLogger.h
 * @brief Logger implementation that delegates logging to spdlog.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 15, 2025
 * @date Last modified: November 15, 2025
 */
#ifndef SK_SPDLOG_LOGGER_H
#define SK_SPDLOG_LOGGER_H

#ifdef USE_SPDLOG

#include <logger/Logger.h>
#include <spdlog/spdlog.h>

namespace sk { namespace logger {

/**
 * @class SpdlogLogger
 * @brief Logger implementation that delegates logging to spdlog.
 *
 * This class wraps the spdlog library and provides a unified interface
 * for logging messages at various severity levels. It is intended to be
 * used via the logging facade (see Logger.h) for backend flexibility.
 */
class SpdlogLogger : public Logger
{
public:
	SpdlogLogger(std::string name);
	~SpdlogLogger(void);

	const std::string getName() { return m_name; }

	bool isFatalEnabled() const;
	bool isErrorEnabled() const;
	bool isWarnEnabled() const;
	bool isInfoEnabled() const;
	bool isDebugEnabled() const;
	bool isTraceEnabled() const;

    Level getLevel(); 
    void setLevel(Level level);

	void fatal(const char *fmt, ...);
	void error(const char *fmt, ...);
	void warn(const char *fmt, ...);
	void info(const char *fmt, ...);
	void debug(const char *fmt, ...);
	void trace(const char *fmt, ...);

	std::shared_ptr<spdlog::logger> getInternalLogger() const { return m_pLogger; }

private:
	std::shared_ptr<spdlog::logger> m_pLogger;
	std::string m_name;
	Level m_level;

	/**
	 * @brief Converts from Logger::Level to spdlog::level::level_enum.
	 * @param level Logger level.
	 * @return Equivalent spdlog level.
	 */
	static spdlog::level::level_enum toSpdlogLevel(Level level);

	/**
	 * @brief Converts from spdlog::level::level_enum to Logger::Level.
	 * @param level spdlog level.
	 * @return Equivalent Logger level.
	 */
	static Level fromSpdlogLevel(spdlog::level::level_enum level);
};

}}

#endif
#endif

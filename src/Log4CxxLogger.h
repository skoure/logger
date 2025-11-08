/**
 * @file Log4CxxLogger.h
 * @brief Logger implementation that delegates logging to Apache Log4cxx.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 * @date Last modified: November 08, 2025
 */
#ifndef SK_LOG4CXX_LOGGER_H
#define SK_LOG4CXX_LOGGER_H

#ifdef USE_LOG4CXX

#include <Logger.h>
#include <log4cxx/logger.h>

#define LOG_MAX_BUF 4096

namespace sk { namespace logger {


/**
 * @class Log4CxxLogger
 * @brief Logger implementation that delegates logging to Apache Log4cxx.
 *
 * This class wraps the Log4cxx library and provides a unified interface
 * for logging messages at various severity levels. It is intended to be
 * used via the logging facade (see Logger.h) for backend flexibility.
 */
class Log4CxxLogger : public Logger
{
public:
	Log4CxxLogger(std::string name);
	~Log4CxxLogger(void);

	const std::string getName() { return m_name; }

	bool isFatalEnabled() const { return m_pLogger->isFatalEnabled(); }
	bool isErrorEnabled() const { return m_pLogger->isErrorEnabled(); }
	bool isWarnEnabled() const { return m_pLogger->isWarnEnabled(); }
	bool isInfoEnabled() const { return m_pLogger->isInfoEnabled(); }
	bool isDebugEnabled() const { return m_pLogger->isDebugEnabled(); }
	bool isTraceEnabled() const { return m_pLogger->isTraceEnabled(); }

    Level getLevel(); 
    void setLevel(Level level);

	void fatal(const char *fmt, ...);
	void error(const char *fmt, ...);
	void warn(const char *fmt, ...);
	void info(const char *fmt, ...);
	void debug(const char *fmt, ...);
	void trace(const char *fmt, ...);

	log4cxx::LoggerPtr getInternalLogger() const { return m_pLogger; }

private:

	log4cxx::LoggerPtr m_pLogger;
	std::string m_name;

};

}}

#endif
#endif
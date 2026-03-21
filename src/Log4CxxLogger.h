/**
 * @file Log4CxxLogger.h
 * @brief Logger implementation that delegates logging to Apache Log4cxx.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 * @date Last modified: March 21, 2026
 */
#ifndef SK_LOG4CXX_LOGGER_H
#define SK_LOG4CXX_LOGGER_H

#include <LoggerBase.h>
#include <log4cxx/logger.h>

namespace sk { namespace logger {

/**
 * @class Log4CxxLogger
 * @brief Logger implementation that delegates logging to Apache Log4cxx.
 *
 * This class wraps the Log4cxx library and provides a unified interface
 * for logging messages at various severity levels. It is intended to be
 * used via the logging facade (see Logger.h) for backend flexibility.
 *
 * All formatting and LogRecord construction is handled by LoggerBase.
 * This class only implements the backend write (append) and level management.
 */
class Log4CxxLogger : public LoggerBase
{
public:
    Log4CxxLogger(std::string name);
    ~Log4CxxLogger();

    const std::string getName() { return m_name; }

    Level getLevel();
    void  setLevel(Level level);

    bool isFatalEnabled() const { return m_pLogger->isFatalEnabled(); }
    bool isErrorEnabled() const { return m_pLogger->isErrorEnabled(); }
    bool isWarnEnabled()  const { return m_pLogger->isWarnEnabled();  }
    bool isInfoEnabled()  const { return m_pLogger->isInfoEnabled();  }
    bool isDebugEnabled() const { return m_pLogger->isDebugEnabled(); }
    bool isTraceEnabled() const { return m_pLogger->isTraceEnabled(); }

    log4cxx::LoggerPtr getInternalLogger() const { return m_pLogger; }

protected:
    void append(const LogRecord& record) override;

private:
    log4cxx::LoggerPtr m_pLogger;
    std::string        m_name;
};

}} // namespace sk::logger

#endif // SK_LOG4CXX_LOGGER_H

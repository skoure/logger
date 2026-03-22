/**
 * @file Log4CxxLogger.h
 * @brief Logger implementation that delegates logging to Apache Log4cxx.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
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

    // Log4cxx handles level inheritance natively via its dot-separated name
    // registry.  Override all four level methods to delegate to log4cxx so
    // that native log4cxx configuration (e.g. from a config file) is honoured.
    Level getLevel() const override;
    void  setLevel(Level level) override;
    void  clearLevel() override;
    bool  isLevelExplicitlySet() const override;

    // Delegate isXxxEnabled to log4cxx's native checks.
    bool isFatalEnabled() const override { return m_pLogger->isFatalEnabled(); }
    bool isErrorEnabled() const override { return m_pLogger->isErrorEnabled(); }
    bool isWarnEnabled()  const override { return m_pLogger->isWarnEnabled();  }
    bool isInfoEnabled()  const override { return m_pLogger->isInfoEnabled();  }
    bool isDebugEnabled() const override { return m_pLogger->isDebugEnabled(); }
    bool isTraceEnabled() const override { return m_pLogger->isTraceEnabled(); }

    log4cxx::LoggerPtr getInternalLogger() const { return m_pLogger; }

protected:
    void append(const LogRecord& record) override;

private:
    log4cxx::LoggerPtr m_pLogger;
    std::string        m_name;
};

}} // namespace sk::logger

#endif // SK_LOG4CXX_LOGGER_H

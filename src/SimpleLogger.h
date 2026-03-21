/**
 * @file SimpleLogger.h
 * @brief Logger implementation that writes messages to std::clog.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 * @date Last modified: March 21, 2026
 */
#ifndef SK_SIMPLELOGGER_H
#define SK_SIMPLELOGGER_H

#include <LoggerBase.h>

namespace sk { namespace logger {

/**
 * @class SimpleLogger
 * @brief Logger implementation that writes messages to std::clog.
 *
 * This class provides a minimal, platform-independent logger for development
 * and testing. It supports all standard log levels and is intended for use
 * via the logging facade (see Logger.h) for backend flexibility.
 *
 * All formatting and LogRecord construction is handled by LoggerBase.
 * This class only implements the backend write (append) and level management.
 */
class SimpleLogger : public LoggerBase
{
public:
    SimpleLogger(std::string name);
    ~SimpleLogger();

    const std::string getName() { return m_name; }

    Level getLevel()            { return m_level; }
    void  setLevel(Level level) { m_level = level; }

    bool isFatalEnabled() const { return (m_level >= Level::Fatal); }
    bool isErrorEnabled() const { return (m_level >= Level::Error); }
    bool isWarnEnabled()  const { return (m_level >= Level::Warn);  }
    bool isInfoEnabled()  const { return (m_level >= Level::Info);  }
    bool isDebugEnabled() const { return (m_level >= Level::Debug); }
    bool isTraceEnabled() const { return (m_level >= Level::Trace); }

protected:
    void append(const LogRecord& record) override;

private:
    const std::string m_name;
    Level             m_level;
};

}} // namespace sk::logger

#endif // SK_SIMPLELOGGER_H

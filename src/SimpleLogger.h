/**
 * @file SimpleLogger.h
 * @brief Logger implementation that writes messages to std::clog.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 * @date Last modified: November 08, 2025
 */
#ifndef SK_SIMPLELOGGER_H
#define SK_SIMPLELOGGER_H

#include <logger/Logger.h>
#include <stdarg.h>

#define LOG_MAX_BUF 4096

namespace sk { namespace logger {

/**
 * @class SimpleLogger
 * @brief Logger implementation that writes messages to std::clog.
 *
 * This class provides a minimal, platform-independent logger for development
 * and testing. It supports all standard log levels and is intended for use
 * via the logging facade (see Logger.h) for backend flexibility.
 */
class SimpleLogger : public Logger {
public:
    SimpleLogger(std::string name);
    ~SimpleLogger();

    const std::string getName() {return m_name;}

    Level getLevel() {return m_level;} 
    void setLevel(Level level) {m_level = level;}

    bool isFatalEnabled() const { return (m_level >= Level::Fatal); }
    bool isErrorEnabled() const { return (m_level >= Level::Error); }
    bool isWarnEnabled() const { return (m_level >= Level::Warn); }
    bool isInfoEnabled() const { return (m_level >= Level::Info); }
    bool isDebugEnabled() const { return (m_level >= Level::Debug); }
    bool isTraceEnabled() const { return (m_level >= Level::Trace); }

    void fatal(const char *fmt, ...);
    void error(const char *fmt, ...);
    void warn(const char *fmt, ...);
    void info(const char *fmt, ...);
    void debug(const char *fmt, ...);
    void trace(const char *fmt, ...);

    void error(const char* msg, const std::exception& ex) override;
    void fatal(const char* msg, const std::exception& ex) override;


private:
    void log(const char* level, const char *fmt, va_list argp);
    const std::string m_name;
    Level m_level;   
};

}}

#endif
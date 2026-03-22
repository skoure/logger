/**
 * @file SimpleLogger.h
 * @brief Logger implementation that writes messages to std::clog.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
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

protected:
    void append(const LogRecord& record) override;

private:
    const std::string m_name;
};

}} // namespace sk::logger

#endif // SK_SIMPLELOGGER_H

/**
 * @file SpdlogLogger.h
 * @brief Logger implementation that delegates logging to spdlog.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 15, 2025
 * @date Last modified: March 21, 2026
 */
#ifndef SK_SPDLOG_LOGGER_H
#define SK_SPDLOG_LOGGER_H

#include <LoggerBase.h>
#include <spdlog/spdlog.h>

namespace sk { namespace logger {

/**
 * @class SpdlogLogger
 * @brief Logger implementation that delegates logging to spdlog.
 *
 * This class wraps the spdlog library and provides a unified interface
 * for logging messages at various severity levels. It is intended to be
 * used via the logging facade (see Logger.h) for backend flexibility.
 *
 * All formatting and LogRecord construction is handled by LoggerBase.
 * This class only implements the backend write (append) and level management.
 */
class SpdlogLogger : public LoggerBase
{
public:
    SpdlogLogger(std::string name);
    ~SpdlogLogger();

    const std::string getName() { return m_name; }

    Level getLevel();
    void  setLevel(Level level);

    bool isFatalEnabled() const;
    bool isErrorEnabled() const;
    bool isWarnEnabled()  const;
    bool isInfoEnabled()  const;
    bool isDebugEnabled() const;
    bool isTraceEnabled() const;

    std::shared_ptr<spdlog::logger> getInternalLogger() const { return m_pLogger; }

protected:
    void append(const LogRecord& record) override;

private:
    std::shared_ptr<spdlog::logger> m_pLogger;
    std::string                     m_name;
    Level                           m_level;

    static spdlog::level::level_enum toSpdlogLevel(Level level);
    static Level fromSpdlogLevel(spdlog::level::level_enum level);
};

}} // namespace sk::logger

#endif // SK_SPDLOG_LOGGER_H

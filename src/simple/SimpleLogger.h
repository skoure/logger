/**
 * @file SimpleLogger.h
 * @brief Logger implementation that writes messages to configurable sinks.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 */
#ifndef SK_SIMPLELOGGER_H
#define SK_SIMPLELOGGER_H

#include <LoggerBase.h>
#include <SimpleSink.h>
#include <ostream>
#include <string>
#include <vector>
#include <mutex>

namespace sk { namespace logger {

/**
 * @class SimpleLogger
 * @brief Logger implementation that writes formatted messages to one or more streams.
 *
 * If no sinks have been configured, append() falls back to std::clog with the
 * hardcoded default format.
 */
class SimpleLogger : public LoggerBase
{
public:
    explicit SimpleLogger(std::string name);
    ~SimpleLogger();

    std::string getName() const override { return m_name; }

    /**
     * @brief Replace all sinks with the provided list.
     * @param sinks New sink list (may be empty — falls back to clog).
     */
    void setSinks(std::vector<SimpleSink> sinks);

    /**
     * @brief Return a read-only reference to the current sink list.
     */
    const std::vector<SimpleSink>& getSinks() const { return m_sinks; }

protected:
    void append(const LogRecord& record) override;

private:
    const std::string      m_name;
    std::vector<SimpleSink> m_sinks;
    std::mutex m_sinkMutex;

    /** Writes one line to @p os using the compiled pattern. */
    void writeToStream(std::ostream& os, const CompiledPattern& pattern,
                       const LogRecord& record) const;
    void writeDefaultToStream(std::ostream& os, const LogRecord& record) const;
};

}} // namespace sk::logger

#endif // SK_SIMPLELOGGER_H

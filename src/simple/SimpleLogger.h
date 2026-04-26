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
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace sk { namespace logger {

/**
 * @struct SimpleSink
 * @brief A single output destination for a SimpleLogger.
 *
 * When pattern is empty the default hardcoded format is used by append().
 */
struct SimpleSink
{
    std::shared_ptr<std::ostream> stream;
    std::string                   pattern; ///< canonical log4j-style; empty → default
    bool                          color = false;
    std::optional<Logger::Level>  level;   ///< nullopt = no additional filtering
};

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

    const std::string getName() { return m_name; }

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

    /** Writes one line to @p os using @p pattern (or the default format). */
    void writeToStream(std::ostream& os,
                       const std::string& pattern,
                       const LogRecord& record) const;
};

}} // namespace sk::logger

#endif // SK_SIMPLELOGGER_H

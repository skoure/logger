/**
 * @file SimpleLogger.cpp
 * @brief Logger implementation that writes formatted messages to configurable sinks.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 */
#include <SimpleLogger.h>
#include <SimpleLoggerPattern.h>
#include <iostream>

using namespace sk::logger;
using namespace std;

static const char* levelToAnsiCode(Logger::Level level)
{
    switch (level)
    {
    case Logger::Level::Fatal: return "\x1b[35m"; // magenta
    case Logger::Level::Error: return "\x1b[31m"; // red
    case Logger::Level::Warn:  return "\x1b[33m"; // yellow
    case Logger::Level::Info:  return "\x1b[32m"; // green
    case Logger::Level::Debug: return "\x1b[36m"; // cyan
    case Logger::Level::Trace: return "\x1b[34m"; // blue
    default:                   return "";
    }
}

SimpleLogger::SimpleLogger(std::string name)
    : m_name(std::move(name))
{
}

SimpleLogger::~SimpleLogger()
{
}

void SimpleLogger::setSinks(std::vector<SimpleSink> sinks)
{
    m_sinks = std::move(sinks);
}

void SimpleLogger::writeToStream(std::ostream& os,
                                 const std::string& pattern,
                                 const LogRecord& record) const
{
    if (pattern.empty())
    {
        // Hardcoded default format
        os << levelToString(record.level)
           << " [" << record.loggerName << "] "
           << record.message << "\n";
    }
    else
    {
        os << SimpleLoggerPattern::render(pattern, record);
    }
}

void SimpleLogger::append(const LogRecord& record)
{
    if (m_sinks.empty())
    {
        writeToStream(clog, "", record);
        auto flushOn = getFlushOn();
        if (flushOn.has_value() && record.level <= *flushOn)
            clog.flush();
        return;
    }

    for (const SimpleSink& sink : m_sinks)
    {
        if (sink.level.has_value() && record.level > *sink.level)
            continue;

        if (sink.stream)
        {
            if (sink.color)
                *sink.stream << levelToAnsiCode(record.level);
            writeToStream(*sink.stream, sink.pattern, record);
            if (sink.color)
                *sink.stream << "\x1b[0m";
        }
    }

    auto flushOn = getFlushOn();
    if (flushOn.has_value() && record.level <= *flushOn)
    {
        for (const SimpleSink& sink : m_sinks)
        {
            if (sink.level.has_value() && record.level > *sink.level)
                continue;
            if (sink.stream) sink.stream->flush();
        }
    }
}

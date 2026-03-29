/**
 * @file SimpleLogger.cpp
 * @brief Logger implementation that writes formatted messages to configurable sinks.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 * @date Last modified: March 28, 2026
 */
#include <SimpleLogger.h>
#include <SimpleLoggerPattern.h>
#include <iostream>

using namespace sk::logger;
using namespace std;

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
        return;
    }

    for (const SimpleSink& sink : m_sinks)
    {
        if (sink.stream)
            writeToStream(*sink.stream, sink.pattern, record);
    }
}

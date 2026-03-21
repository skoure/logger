/**
 * @file SimpleLogger.cpp
 * @brief Minimal logger implementation that writes messages to std::clog.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 * @date Last modified: March 21, 2026
 */
#include <SimpleLogger.h>
#include <iostream>

using namespace sk::logger;
using namespace std;

SimpleLogger::SimpleLogger(std::string name)
    : m_level(Level::Info),
      m_name(std::move(name))
{
}

SimpleLogger::~SimpleLogger()
{
}

void SimpleLogger::append(const LogRecord& record)
{
    clog << levelToString(record.level)
         << " [" << record.loggerName << "] "
         << record.message << endl;
}

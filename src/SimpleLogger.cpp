/**
 * @file SimpleLogger.cpp
 * @brief Minimal logger implementation that writes messages to std::clog.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 * @date Last modified: November 08, 2025
 */
#include <SimpleLogger.h>
#include <iostream>
#include <stdarg.h>

using namespace sk::logger;
using namespace std;

SimpleLogger::SimpleLogger(std::string name) : m_level(Level::Info),
                                               m_name(std::move(name))
{
}

SimpleLogger::~SimpleLogger(void)
{
}

void SimpleLogger::fatal(const char *fmt, ...)
{
    if (isFatalEnabled())
    {
        va_list argp;
        va_start(argp, fmt);
        log("FATAL", fmt, argp);
        va_end(argp);
    }
}

void SimpleLogger::error(const char *fmt, ...)
{
    if (isErrorEnabled())
    {
        va_list argp;
        va_start(argp, fmt);
        log("ERROR", fmt, argp);
        va_end(argp);
    }
}

void SimpleLogger::warn(const char *fmt, ...)
{
    if (isWarnEnabled())
    {
        va_list argp;
        va_start(argp, fmt);
        log("WARN", fmt, argp);
        va_end(argp);
    }
}

void SimpleLogger::info(const char *fmt, ...)
{
    if (isInfoEnabled())
    {
        va_list argp;
        va_start(argp, fmt);
        log("INFO", fmt, argp);
        va_end(argp);
    }
}

void SimpleLogger::debug(const char *fmt, ...)
{
    if (isDebugEnabled())
    {
        va_list argp;
        va_start(argp, fmt);
        log("DEBUG", fmt, argp);
        va_end(argp);
    }
}

void SimpleLogger::trace(const char *fmt, ...)
{
    if (isTraceEnabled())
    {
        va_list argp;
        va_start(argp, fmt);
        log("TRACE", fmt, argp);
        va_end(argp);
    }
}

void SimpleLogger::log(const char *level, const char *fmt, va_list argp)
{
    char buf[LOG_MAX_BUF + 1] = {0};
    std::vsnprintf(buf, LOG_MAX_BUF, fmt, argp);
    clog << level << " [" << getName() << "] " << buf << std::endl;
}

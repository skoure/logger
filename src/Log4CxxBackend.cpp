/**
 * @file Log4CxxBackend.cpp
 * @brief ILoggerBackend implementation for the Log4cxx backend.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#include <Log4CxxBackend.h>
#include <Log4CxxLogger.h>
#include <LoggerFactoryImpl.h>

using namespace sk::logger;

namespace {
    const bool s_registered = []() {
        LoggerFactoryImpl::getInstance().setBackend(std::make_unique<Log4CxxBackend>());
        return true;
    }();
}

LoggerPtr Log4CxxBackend::createLogger(const std::string& name)
{
    return std::make_shared<Log4CxxLogger>(name);
}

void Log4CxxBackend::applyParentSinks(LoggerPtr /*child*/, LoggerPtr /*parent*/)
{
    // Log4cxx manages its own appender inheritance — nothing to do here.
}

void sk::logger::useLog4CxxBackend()
{
    LoggerFactoryImpl::getInstance().setBackend(std::make_unique<Log4CxxBackend>());
}

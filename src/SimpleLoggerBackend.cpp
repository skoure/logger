/**
 * @file SimpleLoggerBackend.cpp
 * @brief ILoggerBackend implementation for the SimpleLogger backend.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#include <SimpleLoggerBackend.h>
#include <SimpleLogger.h>
#include <LoggerFactoryImpl.h>

using namespace sk::logger;

namespace {
    const bool s_registered = []() {
        LoggerFactoryImpl::getInstance().setBackend(std::make_unique<SimpleLoggerBackend>());
        return true;
    }();
}

LoggerPtr SimpleLoggerBackend::createLogger(const std::string& name)
{
    return std::make_shared<SimpleLogger>(name);
}

void SimpleLoggerBackend::applyParentSinks(LoggerPtr /*child*/, LoggerPtr /*parent*/)
{
    // SimpleLogger writes directly to std::clog — no explicit sink objects to copy.
}

void sk::logger::useSimpleLoggerBackend()
{
    LoggerFactoryImpl::getInstance().setBackend(std::make_unique<SimpleLoggerBackend>());
}

/**
 * @file SpdlogBackend.cpp
 * @brief ILoggerBackend implementation for the spdlog backend.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#include <SpdlogBackend.h>
#include <SpdlogLogger.h>
#include <LoggerFactoryImpl.h>

using namespace sk::logger;

namespace {
    const bool s_registered = []() {
        LoggerFactoryImpl::getInstance().setBackend(std::make_unique<SpdlogBackend>());
        return true;
    }();
}

LoggerPtr SpdlogBackend::createLogger(const std::string& name)
{
    return std::make_shared<SpdlogLogger>(name);
}

void SpdlogBackend::applyParentSinks(LoggerPtr child, LoggerPtr parent)
{
    auto childSpdlog  = std::dynamic_pointer_cast<SpdlogLogger>(child);
    auto parentSpdlog = std::dynamic_pointer_cast<SpdlogLogger>(parent);

    if (!childSpdlog || !parentSpdlog) return;

    auto parentInternal = parentSpdlog->getInternalLogger();
    auto childInternal  = childSpdlog->getInternalLogger();

    if (!parentInternal || !childInternal) return;

    childInternal->sinks() = parentInternal->sinks();
}

void sk::logger::useSpdlogBackend()
{
    LoggerFactoryImpl::getInstance().setBackend(std::make_unique<SpdlogBackend>());
}

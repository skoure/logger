/**
 * @file LoggerFactory.cpp
 * @brief Singleton factory implementation for creating and retrieving logger
 * instances.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 * @date Last modified: November 16, 2025
 */
#include "LoggerFactoryImpl.h"

#include <logger/LoggerFactory.h>

#ifdef USE_LOG4CXX
#include <Log4CxxLogger.h>
#define LOGGER(name) Log4CxxLogger(name)
#elif USE_SPDLOG
#include <SpdlogLogger.h>
#define LOGGER(name) SpdlogLogger(name)
#else
#include <SimpleLogger.h>
#define LOGGER(name) SimpleLogger(name)
#endif

using namespace sk::logger;

LoggerFactoryImpl& LoggerFactoryImpl::getInstance() {
    static LoggerFactoryImpl factory;
    return factory;
}

LoggerFactoryImpl::~LoggerFactoryImpl(void) {
    std::lock_guard<std::mutex> lock(m_factoryLock);
    m_loggers.clear();
}

LoggerPtr LoggerFactoryImpl::createLogger(const std::string& name) { 
    return LoggerPtr(new LOGGER(std::move(name))); 
}

LoggerPtr LoggerFactoryImpl::getLogger(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_factoryLock);
    auto it = m_loggers.find(name);
    if (it != m_loggers.end()) return it->second;

    // Create logger
    LoggerPtr pLogger = createLogger(name);
    m_loggers.emplace(name, pLogger);

    return pLogger;
}

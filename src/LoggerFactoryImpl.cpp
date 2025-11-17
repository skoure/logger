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
    m_hierarchy.clear();
}

LoggerPtr LoggerFactoryImpl::createLogger(const std::string& name) { return LoggerPtr(new LOGGER(std::move(name))); }

/**
 * @brief Copy parent sinks to child logger.
 */
#ifdef USE_SPDLOG
bool LoggerFactoryImpl::applyParentSinks(LoggerPtr childLogger, LoggerPtr parentLogger) {
    auto childSpdlog = std::dynamic_pointer_cast<SpdlogLogger>(childLogger);
    auto parentSpdlog = std::dynamic_pointer_cast<SpdlogLogger>(parentLogger);

    if (!parentLogger || !childLogger) {
        return false;
    }

    auto parentInternal = parentSpdlog->getInternalLogger();
    auto childInternal = childSpdlog->getInternalLogger();

    if(!parentInternal || !childInternal) {
        return false;
    }

    childInternal->sinks() = parentInternal->sinks();

    return true;
}
#else
bool LoggerFactoryImpl::applyParentSinks(LoggerPtr childLogger, LoggerPtr parentLogger) { return false; }
#endif

/**
 * @brief Copies log level and sinks from parent to child
 */
bool LoggerFactoryImpl::applyParentConfiguration(LoggerPtr childLogger, LoggerPtr parentLogger) {
    if (!childLogger || !parentLogger) return false;

    // Inherit log level from parent
    childLogger->setLevel(parentLogger->getLevel());

    // Inherit appenders/sinks from parent logger if applicable
    if (m_hierarchy.hasAdditivity(childLogger->getName())) {
        return applyParentSinks(childLogger, parentLogger);
    }
    return true;
}

LoggerPtr LoggerFactoryImpl::getLogger(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_factoryLock);
    LoggerPtr logger = m_hierarchy.getLogger(name);

    if (logger) {
        return logger;
    }

    // Create logger
    LoggerPtr pLogger = createLogger(name);

    // Add to hierarchy
    m_hierarchy.addLogger(name, pLogger, true);

    // Log4cxx, levels and appenders are handled by the framework, so no need
    // to do anything special
#ifdef USE_LOG4CXX
    return pLogger;
#endif

    // Inherit configuration from parent logger if it exists
    auto parentLogger = m_hierarchy.getParent(name);
    if (parentLogger) {
        applyParentConfiguration(pLogger, parentLogger);
    }

    return pLogger;
}
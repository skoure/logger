/**
 * @file LoggerFactoryImpl.cpp
 * @brief Singleton factory implementation for creating and retrieving logger instances.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 * @date Last modified: March 21, 2026
 */
#include <LoggerFactoryImpl.h>
#include <logger/LoggerFactory.h>

using namespace sk::logger;

LoggerFactoryImpl& LoggerFactoryImpl::getInstance()
{
    static LoggerFactoryImpl factory;
    return factory;
}

LoggerFactoryImpl::~LoggerFactoryImpl()
{
    std::lock_guard<std::mutex> lock(m_factoryLock);
    m_hierarchy.clear();
}

void LoggerFactoryImpl::setBackend(std::unique_ptr<ILoggerBackend> backend)
{
    std::lock_guard<std::mutex> lock(m_factoryLock);
    m_backend = std::move(backend);
}

bool LoggerFactoryImpl::applyParentConfiguration(LoggerPtr childLogger, LoggerPtr parentLogger)
{
    if (!childLogger || !parentLogger) return false;

    childLogger->setLevel(parentLogger->getLevel());

    if (m_backend) {
        m_backend->applyParentSinks(childLogger, parentLogger);
    }
    return true;
}

LoggerPtr LoggerFactoryImpl::getLogger(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_factoryLock);

    LoggerPtr logger = m_hierarchy.getLogger(name);
    if (logger) return logger;

    if (!m_backend) return nullptr;

    LoggerPtr pLogger = m_backend->createLogger(name);
    m_hierarchy.addLogger(name, pLogger, true);

    if (m_backend->supportsNativeHierarchy()) {
        return pLogger;
    }

    auto parentLogger = m_hierarchy.getParent(name);
    if (parentLogger) {
        applyParentConfiguration(pLogger, parentLogger);
    }

    return pLogger;
}

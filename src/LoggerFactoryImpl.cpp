/**
 * @file LoggerFactoryImpl.cpp
 * @brief Singleton factory implementation for creating and retrieving logger instances.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 */
#include <LoggerFactoryImpl.h>
#include <LoggerBase.h>
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

LoggerPtr LoggerFactoryImpl::getLogger(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_factoryLock);

    LoggerPtr logger = m_hierarchy.getLogger(name);
    if (logger) return logger;

    if (!m_backend) return nullptr;

    LoggerPtr pLogger = m_backend->createLogger(name);
    m_hierarchy.addLogger(name, pLogger);

    // Wire parent pointer so getLevel() can walk up the chain dynamically.
    LoggerPtr parentLogger = m_hierarchy.getParent(name);
    if (parentLogger) {
        auto* base = dynamic_cast<LoggerBase*>(pLogger.get());
        if (base) base->setParent(parentLogger);

        // For backends without native sink inheritance, propagate sinks.
        if (!m_backend->supportsNativeHierarchy()) {
            m_backend->applyParentSinks(pLogger, parentLogger);
        }
    }

    return pLogger;
}

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
#include <LazyLogger.h>
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

void LoggerFactory::setLevelNames(const LevelNames& names)
{
    LoggerBase::setLevelNames(names);
}

void LoggerFactoryImpl::setBackend(std::unique_ptr<ILoggerBackend> backend)
{
    std::lock_guard<std::mutex> lock(m_factoryLock);
    m_backend = std::move(backend);
}

void LoggerFactoryImpl::configureLogger(LoggerPtr logger,
                                         const std::vector<SinkConfig>& sinks)
{
    std::lock_guard<std::mutex> lock(m_factoryLock);
    if (m_backend && logger)
        m_backend->configureLogger(logger, sinks);
}

void LoggerFactory::configureLoggerWithOstream(const char* name, std::ostream& os,
                                               const std::string& canonicalPattern)
{
    LoggerFactoryImpl& impl = LoggerFactoryImpl::getInstance();
    LoggerPtr logger = impl.getLogger(name);
    if (logger)
        impl.configureLoggerWithOstream(logger, os, canonicalPattern);
}

void LoggerFactoryImpl::configureLoggerWithOstream(LoggerPtr logger, std::ostream& os,
                                                   const std::string& canonicalPattern)
{
    std::lock_guard<std::mutex> lock(m_factoryLock);
    if (m_backend && logger)
        m_backend->configureLoggerWithOstream(logger, os, canonicalPattern);
}

LoggerPtr LoggerFactoryImpl::getLogger(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_factoryLock);

    LoggerPtr logger = m_hierarchy.getLogger(name);
    if (logger) return logger;

    // Fast path: backend ready — create real logger directly (no proxy overhead).
    if (m_backend) {
        LoggerPtr pLogger = m_backend->createLogger(name);
        m_hierarchy.addLogger(name, pLogger);

        LoggerPtr parentLogger = m_hierarchy.getParent(name);
        if (parentLogger) {
            auto* base = dynamic_cast<LoggerBase*>(pLogger.get());
            if (base) base->setParent(parentLogger);

            if (!m_backend->supportsNativeHierarchy())
                m_backend->applyParentSinks(pLogger, parentLogger);
        }
        return pLogger;
    }

    // SIOF window: backend not yet registered — return a lazy proxy.
    // The LazyLogger defers real backend creation until append() is first called,
    // by which point all static initializers have run and the backend is ready.
    auto pLazy = std::make_shared<LazyLogger>(name);
    m_hierarchy.addLogger(name, pLazy);

    // Wire parent for level inheritance. Parent may also be a LazyLogger; that's
    // fine — LoggerBase::getLevel() walks the weak_ptr chain regardless of type.
    LoggerPtr parentLogger = m_hierarchy.getParent(name);
    if (parentLogger)
        pLazy->setParent(parentLogger);
    // No applyParentSinks — LazyLogger has no sinks; deferred to createBackendLogger.

    return pLazy;
}

LoggerPtr LoggerFactoryImpl::createBackendLogger(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_factoryLock);
    if (!m_backend) return nullptr;

    LoggerPtr pLogger = m_backend->createLogger(name);

    LoggerPtr parentLogger = m_hierarchy.getParent(name);
    if (parentLogger) {
        auto* base = dynamic_cast<LoggerBase*>(pLogger.get());
        if (base) base->setParent(parentLogger);

        if (!m_backend->supportsNativeHierarchy())
            m_backend->applyParentSinks(pLogger, parentLogger);
    }
    return pLogger;
}

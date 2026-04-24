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
    m_sinkManagedBackend = dynamic_cast<IManagedSinkBackend*>(m_backend.get());
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

// Returns the ancestor names of `name` that have no real logger assigned yet,
// ordered shallowest-first so each can be created before its children.
// Stops as soon as it finds an ancestor that already exists in the hierarchy.
static std::vector<std::string> missingAncestors(const std::string& name,
                                                  const LoggerHierarchy& h)
{
    std::vector<std::string> missing;
    std::string path = name;
    while (true) {
        size_t dot = path.find_last_of('.');
        if (dot == std::string::npos) break;  // parent would be "root"
        path = path.substr(0, dot);
        if (h.getLogger(path)) break;         // exists — its ancestors must too
        missing.push_back(path);
    }
    std::reverse(missing.begin(), missing.end());
    return missing;
}

LoggerPtr LoggerFactoryImpl::getLogger(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_factoryLock);

    LoggerPtr logger = m_hierarchy.getLogger(name);
    if (logger) return logger;

    // Fast path: backend ready — create real loggers for any missing intermediate
    // ancestors first, then create the requested logger. Every node in the
    // hierarchy then has a real LoggerPtr so getParent() always succeeds.
    if (m_backend) {
        for (const auto& ancestorName : missingAncestors(name, m_hierarchy)) {
            LoggerBasePtr pAncestor = m_backend->createLogger(ancestorName);
            m_hierarchy.addLogger(ancestorName, pAncestor);
            LoggerPtr parent = m_hierarchy.getParent(ancestorName);
            if (parent) {
                pAncestor->setParent(parent);
                if (m_sinkManagedBackend)
                    m_sinkManagedBackend->applyParentSinks(pAncestor, parent);
            }
        }

        LoggerBasePtr pLogger = m_backend->createLogger(name);
        m_hierarchy.addLogger(name, pLogger);

        LoggerPtr parentLogger = m_hierarchy.getParent(name);
        if (parentLogger) {
            pLogger->setParent(parentLogger);
            if (m_sinkManagedBackend)
                m_sinkManagedBackend->applyParentSinks(pLogger, parentLogger);
        }
        return pLogger;
    }

    // SIOF window: backend not yet registered — return a lazy proxy.
    // Create LazyLogger instances for any missing intermediate ancestors so the
    // parent chain is fully wired and getParent() works for all descendants.
    for (const auto& ancestorName : missingAncestors(name, m_hierarchy)) {
        auto pAncestor = std::make_shared<LazyLogger>(ancestorName);
        m_hierarchy.addLogger(ancestorName, pAncestor);
        LoggerPtr parent = m_hierarchy.getParent(ancestorName);
        if (parent) pAncestor->setParent(parent);
    }

    auto pLazy = std::make_shared<LazyLogger>(name);
    m_hierarchy.addLogger(name, pLazy);

    LoggerPtr parentLogger = m_hierarchy.getParent(name);
    if (parentLogger)
        pLazy->setParent(parentLogger);

    return pLazy;
}

LoggerBasePtr LoggerFactoryImpl::createBackendLogger(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_factoryLock);
    if (!m_backend) return nullptr;

    LoggerBasePtr pLogger = m_backend->createLogger(name);

    // KNOWN LIMITATION: if this logger's name appeared in a configure() JSON
    // with explicit sinks while it was still a LazyLogger, those explicit sinks
    // are not replayed here — configureLogger() was a no-op on the proxy.
    // The logger receives inherited sinks from its direct parent instead.
    LoggerPtr parentLogger = m_hierarchy.getParent(name);
    if (parentLogger) {
        pLogger->setParent(parentLogger);

        if (m_sinkManagedBackend)
            m_sinkManagedBackend->applyParentSinks(pLogger, parentLogger);
    }
    return pLogger;
}

void LoggerFactoryImpl::clearAllLevels()
{
    std::lock_guard<std::mutex> lock(m_factoryLock);
    for (const auto& entry : m_hierarchy.getAllLoggersTopDown())
        entry.second->clearLevel();
}

void LoggerFactoryImpl::clearAllSinks()
{
    std::lock_guard<std::mutex> lock(m_factoryLock);
    if (!m_sinkManagedBackend) return;
    for (const auto& entry : m_hierarchy.getAllLoggersTopDown())
        m_sinkManagedBackend->clearSinks(entry.second);
}

void LoggerFactoryImpl::clearAllFlushOn()
{
    std::lock_guard<std::mutex> lock(m_factoryLock);
    for (const auto& entry : m_hierarchy.getAllLoggersTopDown())
        entry.second->clearFlushOn();
}

void LoggerFactoryImpl::propagateInheritedSinks(const std::set<std::string>& configured)
{
    std::lock_guard<std::mutex> lock(m_factoryLock);
    if (!m_sinkManagedBackend) return;
    for (const auto& entry : m_hierarchy.getAllLoggersTopDown()) {
        if (configured.count(entry.first)) continue;
        LoggerPtr parent = m_hierarchy.getParent(entry.first);
        if (parent)
            m_sinkManagedBackend->applyParentSinks(entry.second, parent);
    }
}

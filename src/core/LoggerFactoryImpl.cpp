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
#include <ProxyLogger.h>
#include <LoggerBase.h>
#include <logger/LoggerFactory.h>

using namespace sk::logger;

LoggerFactoryImpl& LoggerFactoryImpl::getInstance()
{
    static LoggerFactoryImpl factory;
    return factory;
}

LoggerFactoryImpl::LoggerFactoryImpl()
{
    m_proxyBackend = std::make_unique<ProxyBackend>();
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

     // Materialise every proxy so all m_real pointers are non-null.
     // Use backend directly (before moving) so each proxy receives a real
     // backend logger
     for (const auto& [name, logger] : m_hierarchy.getAllLoggersTopDown()) {
         if (auto proxy = std::dynamic_pointer_cast<ProxyLogger>(logger)) {
             if (!proxy->getReal()) {
                 auto real = backend->createLogger(name);
                 proxy->setReal(real);
             }
         }
     }

     m_proxyBackend->setRealBackend(std::move(backend));
}

void LoggerFactoryImpl::configureLogger(LoggerPtr logger,
                                         const std::vector<SinkConfig>& sinks)
{
    std::lock_guard<std::mutex> lock(m_factoryLock);
    if (logger) {
        m_proxyBackend->configureLogger(logger, sinks);
	}
}

void LoggerFactory::configureLoggerWithOstream(const char* name, std::ostream& os,
                                               const std::string& canonicalPattern)
{
    LoggerFactoryImpl& impl = LoggerFactoryImpl::getInstance();
    LoggerPtr logger = impl.getLogger(name);
    if (logger) {
        impl.configureLoggerWithOstream(logger, os, canonicalPattern);
	}
}

void LoggerFactoryImpl::configureLoggerWithOstream(LoggerPtr logger, std::ostream& os,
                                                   const std::string& canonicalPattern)
{
    std::lock_guard<std::mutex> lock(m_factoryLock);
    if (logger) {
        m_proxyBackend->configureLoggerWithOstream(logger, os, canonicalPattern);
	}
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
        if (dot == std::string::npos) {
            // Reached a top-level name — its parent is root.
            // Include root if it has no logger and we are not computing
            // ancestors for root itself.
            if (path != "root" && !h.getLogger("root"))
                missing.push_back("root");
            break;
        }
        path = path.substr(0, dot);
        if (h.getLogger(path)) break; // exists — its ancestors must too
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

    // m_proxyBackend is always set (ProxyBackend from construction). During the SIOF
    // window ProxyBackend::createLogger returns a ProxyLogger; after the real
    // backend registers it returns a real logger. Both paths are identical here.
    for (const auto& ancestorName : missingAncestors(name, m_hierarchy)) {
        LoggerBasePtr pAncestor = m_proxyBackend->createLogger(ancestorName);
        m_hierarchy.addLogger(ancestorName, pAncestor);
        LoggerPtr parent = m_hierarchy.getParent(ancestorName);
        if (parent) {
            pAncestor->setParent(parent);
            m_proxyBackend->applyParentSinks(pAncestor, parent);
        }
    }

    LoggerBasePtr pLogger = m_proxyBackend->createLogger(name);
    m_hierarchy.addLogger(name, pLogger);

    LoggerPtr parentLogger = m_hierarchy.getParent(name);
    if (parentLogger) {
        pLogger->setParent(parentLogger);
        m_proxyBackend->applyParentSinks(pLogger, parentLogger);
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
    for (const auto& entry : m_hierarchy.getAllLoggersTopDown()) {
        m_proxyBackend->clearSinks(entry.second);
	}
}

void LoggerFactoryImpl::clearAllFlushOn()
{
    std::lock_guard<std::mutex> lock(m_factoryLock);
    for (const auto& entry : m_hierarchy.getAllLoggersTopDown()) {
        entry.second->clearFlushOn();
	}
}

void LoggerFactoryImpl::propagateInheritedSinks(const std::set<std::string>& configured)
{
    std::lock_guard<std::mutex> lock(m_factoryLock);
    for (const auto& entry : m_hierarchy.getAllLoggersTopDown()) {
        if (configured.count(entry.first)) continue;
        LoggerPtr parent = m_hierarchy.getParent(entry.first);
        if (parent) {
			m_proxyBackend->applyParentSinks(entry.second, parent);
		}     
    }
}

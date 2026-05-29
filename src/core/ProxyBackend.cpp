/**
 * @file ProxyBackend.cpp
 * @brief Backend proxy installed at factory construction to eliminate the null-backend window.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: May 27, 2026
 */
#include <ProxyBackend.h>
#include <ProxyLogger.h>

using namespace sk::logger;

ProxyBackend::ProxyBackend(std::unique_ptr<ILoggerBackend> real)
{
    setRealBackend(std::move(real));
}

void ProxyBackend::setRealBackend(std::unique_ptr<ILoggerBackend> real)
{
    m_realSink = dynamic_cast<IManagedSinkBackend*>(real.get());
    m_real     = std::move(real);
}

LoggerBasePtr ProxyBackend::createLogger(const std::string& name)
{
    LoggerBasePtr realLogger; 
	// Real backend may not be registered yet at creation time; ProxyLogger
    // will be wired up later when LoggerFactoryImpl::setBackend() is called.
	if (m_real) {
		realLogger = m_real->createLogger(name);
	}
    return std::make_shared<ProxyLogger>(name, realLogger);
}

void ProxyBackend::configureLogger(LoggerPtr logger,
                                   const std::vector<SinkConfig>& sinks)
{
    if (!m_real) {
		return;
	}

    // If the logger is a ProxyLogger, we need to unwrap it to get the real logger before forwarding the call to the real backend.  This ensures that the
    // real backend's configureLoggerWithOstream() method receives the correct logger instance, allowing it to apply the configuration properly to the real logger rather than the proxy
    if (auto proxy = std::dynamic_pointer_cast<ProxyLogger>(logger)) {
        auto real = proxy->getReal();
        if (!real) return;
        m_real->configureLogger(real, sinks);
    } else {
        m_real->configureLogger(logger, sinks);
    }
}

void ProxyBackend::configureLoggerWithOstream(LoggerPtr logger, std::ostream& os, const std::string& canonicalPattern) {
    if (!m_real) {
        return;  // real backend not yet registered
    }

    // If the logger is a ProxyLogger, we need to unwrap it to get the real logger before forwarding the call to the
    // real backend.  This ensures that the real backend's configureLoggerWithOstream() method receives the correct
    // logger instance, allowing it to apply the configuration properly to the real logger rather than the proxy.
    if (auto proxy = std::dynamic_pointer_cast<ProxyLogger>(logger)) {
        auto real = proxy->getReal();
        if (!real) return;
        m_real->configureLoggerWithOstream(real, os, canonicalPattern);
    } else {
        m_real->configureLoggerWithOstream(logger, os, canonicalPattern);
    }
}

void ProxyBackend::applyParentSinks(LoggerPtr child, LoggerPtr parent)
{
    if(!m_realSink){
        return; // real backend not yet registered or doesn't support managed sinks
    }
    // If either logger is a ProxyLogger, we need to unwrap it to get the real logger before forwarding the call to the
    // real backend.  This ensures that the real backend's applyParentSinks() method receives the correct logger
    // instances, allowing it to apply the configuration properly to the real loggers rather than the proxies.
    if (auto proxyChild = std::dynamic_pointer_cast<ProxyLogger>(child)) {
        child = proxyChild->getReal();
        if (!child) return;
    }
    if (auto proxyParent = std::dynamic_pointer_cast<ProxyLogger>(parent)) {
        parent = proxyParent->getReal();
        if (!parent) return;
    }
    m_realSink->applyParentSinks(child, parent);
}

void ProxyBackend::clearSinks(LoggerPtr logger)
{
    if(!m_realSink){
        return; // real backend not yet registered or doesn't support managed sinks
    }
    if (auto proxy = std::dynamic_pointer_cast<ProxyLogger>(logger)) {
        auto real = proxy->getReal();
        if (real) m_realSink->clearSinks(real);
    } else {
        m_realSink->clearSinks(logger);
    }
}

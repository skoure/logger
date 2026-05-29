/**
 * @file ProxyBackend.h
 * @brief Backend proxy installed at factory construction to eliminate the null-backend window.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: May 27, 2026
 */
#ifndef SK_PROXY_BACKEND_H
#define SK_PROXY_BACKEND_H

#include <IManagedSinkBackend.h>
#include <ILoggerBackend.h>
#include <memory>

namespace sk { namespace logger {

/**
 * @class ProxyBackend
 * @brief Backend proxy installed at factory construction so m_backend is never null.
 *
 * During the SIOF window (before the real backend registers), createLogger()
 * returns a ProxyLogger with a null m_real. Once setRealBackend() is called,
 * LoggerFactoryImpl eagerly materialises all existing proxies by calling
 * setReal() on each. Subsequent createLogger() calls still return a ProxyLogger,
 * but with m_real already populated from the real backend.
 *
 * configureLogger() detects ProxyLogger instances and reuses the m_real already
 * set by setBackend(), then forwards the sink configuration through the real
 * backend. This ensures any cached LoggerPtr sees the correct configured level
 * and sinks via ProxyLogger's transparent delegation.
 *
 * applyParentSinks() and clearSinks() unwrap both child and parent ProxyLogger
 * m_real pointers before delegating to the real backend so its internal casts
 * (e.g. to SpdlogLogger*) succeed.
 */
class ProxyBackend : public IManagedSinkBackend
{
public:
    ProxyBackend() = default;
    explicit ProxyBackend(std::unique_ptr<ILoggerBackend> real);

    // Called by LoggerFactoryImpl::setBackend when the real backend registers.
    void setRealBackend(std::unique_ptr<ILoggerBackend> real);

    bool hasRealBackend() const { return m_real != nullptr; }

    // ILoggerBackend
    LoggerBasePtr createLogger(const std::string& name) override;

    void configureLogger(LoggerPtr logger,
                         const std::vector<SinkConfig>& sinks) override;

    void configureLoggerWithOstream(LoggerPtr logger, std::ostream& os,
                                    const std::string& canonicalPattern) override;

    // IManagedSinkBackend
    void applyParentSinks(LoggerPtr child, LoggerPtr parent) override;
    void clearSinks(LoggerPtr logger) override;

private:
    std::unique_ptr<ILoggerBackend>  m_real;
    IManagedSinkBackend*             m_realSink = nullptr;
};

}} // namespace sk::logger

#endif // SK_PROXY_BACKEND_H

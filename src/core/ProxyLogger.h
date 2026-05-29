/**
 * @file ProxyLogger.h
 * @brief Proxy logger that stands in for a real backend logger during the SIOF window.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: April 23, 2026
 */
#ifndef SK_PROXY_LOGGER_H
#define SK_PROXY_LOGGER_H

#include <LoggerBase.h>
#include <string>

namespace sk { namespace logger {

/**
 * @class ProxyLogger
 * @brief Proxy logger that stands in for a real backend logger during the SIOF window.
 *
 * ## Problem: Static Initialization Order Fiasco (SIOF)
 *
 * C++ guarantees that static variables within a single translation unit (TU)
 * are initialized top-to-bottom, but the order across TUs is unspecified.
 * This creates a hazard when a static LoggerPtr is declared in one TU and the
 * backend registers itself (via its own static initializer) in another:
 *
 *   // your_module.cpp
 *   static LoggerPtr logger = LoggerFactory::getLogger("App"); // TU A
 *
 *   // SpdlogBackend.cpp
 *   static bool s_reg = []{ LoggerFactoryImpl::getInstance()
 *                               .setBackend(...); return true; }(); // TU B
 *
 * If TU A initializes before TU B, the backend is not yet registered when
 * getLogger() is called, so the factory (via ProxyBackend) returns a
 * ProxyLogger instead. The proxy remains the authoritative hierarchy entry
 * and is never replaced; user code holding a cached LoggerPtr continues to
 * call the same object for the lifetime of the program.
 *
 * ## Materialisation
 *
 * When the real backend registers, LoggerFactoryImpl::setBackend() walks the
 * hierarchy and calls setReal() on every existing ProxyLogger, wiring each
 * proxy to a newly created real backend logger. From that point all level
 * queries and log calls forward transparently to m_real.
 *
 * When configure() subsequently runs, ProxyBackend::configureLogger() calls
 * setReal() again for loggers named in the JSON, replacing the basic real
 * logger with one that has the configured sinks attached.
 *
 * ## Fallback Behavior
 *
 * In the rare case where append() fires before the real backend registers
 * (e.g., a static initializer that logs before the backend's own static
 * init runs), m_real is null. Rather than silently dropping the record,
 * the level and message are written to std::cerr so the condition is
 * visible during development.
 *
 * ## Thread Safety
 *
 * setReal() is called from LoggerFactoryImpl::setBackend() and
 * LoggerConfigurator::configure(), both of which hold m_factoryLock and
 * are expected to complete before concurrent logging begins. Subsequent
 * reads of m_real in append() and the level accessors are safe without
 * an additional mutex.
 */
class ProxyLogger : public LoggerBase
{
public:
    explicit ProxyLogger(std::string name, LoggerBasePtr real = 0);
    std::string getName() const override;
    void append(const LogRecord& record) override;

	void setParent(std::weak_ptr<Logger> parent) override;
    // Transparent level delegation: once m_real is set, route all level
    // queries and mutations through it so callers holding a cached ProxyLogger
    // pointer see the same level state as if they held the real logger.
    Level getLevel() const override;
    void  setLevel(Level level) override;
    void  clearLevel() override;
    bool  isLevelExplicitlySet() const override;

    void setFlushOn(Level level) override;
    void clearFlushOn() override;
    std::optional<Level> getFlushOn() const override;

    // Called by LoggerFactoryImpl::setBackend() (for all proxies in the hierarchy)
    // and by ProxyBackend::configureLogger() (for JSON-configured loggers) to wire
    // the proxy to a real backend logger.  Forwards any level/flush-on state the
    // proxy accumulated before the backend was registered.
    void          setReal(LoggerBasePtr real);
    LoggerBasePtr getReal() const;

private:
    const std::string  m_name;
    LoggerBasePtr      m_real;  // null only in the SIOF window before setBackend() runs
};

}} // namespace sk::logger

#endif // SK_PROXY_LOGGER_H

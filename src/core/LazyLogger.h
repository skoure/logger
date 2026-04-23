/**
 * @file LazyLogger.h
 * @brief Proxy logger that defers real backend creation until the first log call.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: April 23, 2026
 */
#ifndef SK_LAZY_LOGGER_H
#define SK_LAZY_LOGGER_H

#include <LoggerBase.h>
#include <mutex>
#include <string>

namespace sk { namespace logger {

/**
 * @class LazyLogger
 * @brief Proxy logger that defers real backend creation until the first log call.
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
 * getLogger() is called, historically causing it to return nullptr and any
 * subsequent log call to segfault.
 *
 * ## Solution
 *
 * LoggerFactoryImpl::getLogger() returns a LazyLogger when no backend is
 * registered. The LazyLogger stores the logger name and defers the real
 * backend logger creation until append() is first called. By that point,
 * the program is in main() (or later), all static initializers have run,
 * and the backend is guaranteed to be registered.
 *
 * ## Fallback Behavior
 *
 * In the rare case where append() is called from inside another static
 * initializer (e.g., a static object whose constructor logs), the backend
 * may still not be registered. If createBackendLogger() returns nullptr,
 * the log record cannot be written to the real backend. Rather than silently
 * dropping it, the level and message are written to std::cerr so the
 * condition is visible during development.
 *
 * ## Thread Safety
 *
 * Real backend creation is performed exactly once via std::call_once, which
 * provides a happens-before edge to all threads. Subsequent reads of m_real
 * in append() are safe without an additional mutex.
 */
class LazyLogger : public LoggerBase
{
public:
    explicit LazyLogger(std::string name);
    const std::string getName() override;
    void append(const LogRecord& record) override;

private:
    const std::string m_name;
    LoggerPtr         m_real;      // null until first append(); written once via call_once
    std::once_flag    m_initFlag;
};

}} // namespace sk::logger

#endif // SK_LAZY_LOGGER_H

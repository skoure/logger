/**
 * @file LazyLogger.cpp
 * @brief Proxy logger that defers real backend creation until the first log call.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: April 23, 2026
 */
#include <LazyLogger.h>
#include <LoggerFactoryImpl.h>
#include <iostream>

using namespace sk::logger;

LazyLogger::LazyLogger(std::string name)
    : m_name(std::move(name))
{}

const std::string LazyLogger::getName()
{
    return m_name;
}

void LazyLogger::append(const LogRecord& record)
{
    std::call_once(m_initFlag, [this]() {
        m_real = LoggerFactoryImpl::getInstance().createBackendLogger(m_name);
    });

    if (m_real) {
        m_real->append(record);
    } else {
        // Backend still not registered (called from a static initializer before
        // the backend's own static init ran). Write to cerr so the message is
        // not silently lost and the misconfiguration is visible.
        // std::cerr is safe here — it is guaranteed initialized before any
        // user-defined statics via ios_base::Init.
        std::cerr << "[LOGGER " << LoggerBase::levelToString(record.level)
                  << "] No backend registered - (" << m_name << ") " << record.message << "\n";
    }
}

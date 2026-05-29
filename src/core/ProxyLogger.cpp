/**
 * @file ProxyLogger.cpp
 * @brief Proxy logger that stands in for a real backend logger during the SIOF window.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: April 23, 2026
 */
#include <ProxyLogger.h>
#include <LoggerUtils.h>
#include <iostream>

using namespace sk::logger;

ProxyLogger::ProxyLogger(std::string name, LoggerBasePtr real)
    : m_name(std::move(name)), m_real(std::move(real))
{}

std::string ProxyLogger::getName() const
{
    return m_name;
}

void ProxyLogger::setReal(LoggerBasePtr real)
{
    if (!real) { 
		return; 
	}
    if (LoggerBase::isLevelExplicitlySet()){
        real->setLevel(LoggerBase::getLevel()); 
	}
    auto flushOn = LoggerBase::getFlushOn();
    if (flushOn.has_value()) {
        real->setFlushOn(*flushOn); 
	}
	real->setParent(m_parent);
    m_real = std::move(real);	
}

LoggerBasePtr ProxyLogger::getReal() const
{
    return m_real;
}

void ProxyLogger::setParent(std::weak_ptr<Logger> parent)
{
    LoggerBase::setParent(parent);
	if (m_real) {
		m_real->setParent(parent);
	}
}

Logger::Level ProxyLogger::getLevel() const
{
    if (m_real) {
		return m_real->getLevel();
	}
    return LoggerBase::getLevel();
}

void ProxyLogger::setLevel(Level level)
{
    LoggerBase::setLevel(level);
    if (m_real) {
		m_real->setLevel(level);
	}
}

void ProxyLogger::clearLevel()
{
    LoggerBase::clearLevel();
    if (m_real) {
		m_real->clearLevel();
	}
}

void ProxyLogger::setFlushOn(Level level)
{
    LoggerBase::setFlushOn(level);
    if (m_real) {
        m_real->setFlushOn(level);
    }
}
    
void ProxyLogger::clearFlushOn() 
{
    LoggerBase::clearFlushOn();
    if (m_real) {
        m_real->clearFlushOn();
    }
}
    
std::optional<Logger::Level> ProxyLogger::getFlushOn() const
{
    if (m_real) {
        return m_real->getFlushOn();
    }
    return LoggerBase::getFlushOn();
}

bool ProxyLogger::isLevelExplicitlySet() const
{
    if (m_real) {
		return m_real->isLevelExplicitlySet();
	}
    return LoggerBase::isLevelExplicitlySet();
}

void ProxyLogger::append(const LogRecord& record)
{
    if (m_real) {
        m_real->append(record);
    } else {
        // Backend still not registered (called from a static initializer that logged
        // before the backend's own static init ran). Write to cerr so the message is
        // not silently lost and the misconfiguration is visible during development.
        // std::cerr is safe here — it is guaranteed initialized before any
        // user-defined statics via ios_base::Init.
        std::cerr << "[LOGGER " << LoggerBase::levelToString(record.level)
                  << "] No backend registered - (" << m_name << ") " << record.message << eol;
    }
}

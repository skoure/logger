/**
 * @file Log4CxxLogger.cpp
 * @brief Logger implementation that delegates logging to Apache Log4cxx.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 * @date Last modified: November 08, 2025
 */
#ifdef USE_LOG4CXX

#include "Log4CxxLogger.h"
#include <LoggerUtils.h>
#include <stdarg.h>

using namespace sk::logger;

Log4CxxLogger::Log4CxxLogger(std::string name)
{
	m_name = std::move(name);
    m_pLogger = log4cxx::Logger::getLogger(m_name);
}

Log4CxxLogger::~Log4CxxLogger(void)
{
}


Logger::Level Log4CxxLogger::getLevel(){
    log4cxx::LevelPtr levelPtr = m_pLogger->getLevel();
    if(levelPtr == log4cxx::Level::getFatal()){
        return Level::Fatal;
    } else if(levelPtr == log4cxx::Level::getError()) {
        return Level::Error;
    } else if(levelPtr == log4cxx::Level::getWarn()) {
        return Level::Warn;
    } else if(levelPtr == log4cxx::Level::getInfo()) {
        return Level::Info;
    } else if(levelPtr == log4cxx::Level::getDebug()) {
        return Level::Debug;
    } else if(levelPtr == log4cxx::Level::getTrace()) {
        return Level::Trace;
    }

    // should never get here
    return Level::Info; 
}

void Log4CxxLogger::setLevel(Level level){
    switch(level){
        case Level::Fatal:
            m_pLogger->setLevel(log4cxx::Level::getFatal());
            break;
        case Level::Error:
            m_pLogger->setLevel(log4cxx::Level::getError());
            break;
        case Level::Warn:
            m_pLogger->setLevel(log4cxx::Level::getWarn());
            break;
        case Level::Info:
            m_pLogger->setLevel(log4cxx::Level::getInfo());
            break;
        case Level::Debug:
            m_pLogger->setLevel(log4cxx::Level::getDebug());
            break;
        case Level::Trace:
            m_pLogger->setLevel(log4cxx::Level::getTrace());
            break;
    }

}

void Log4CxxLogger::fatal(const char *fmt, ...){
	if(m_pLogger && this->isFatalEnabled()){
		char buf[LOG_MAX_BUF + 1] = {0};
		va_list ap;
		va_start(ap, fmt);
		std::vsnprintf(buf, LOG_MAX_BUF, fmt, ap);
		LOG4CXX_FATAL(m_pLogger, buf);
		va_end(ap);		
	}
}

void Log4CxxLogger::error(const char *fmt, ...){
	if(m_pLogger && this->isErrorEnabled()){
		char buf[LOG_MAX_BUF + 1] = {0};
		va_list ap;
		va_start(ap, fmt);
		std::vsnprintf(buf, LOG_MAX_BUF, fmt, ap);
		LOG4CXX_ERROR(m_pLogger, buf);
		va_end(ap);		
	}
}

void Log4CxxLogger::warn(const char *fmt, ...){
	if(m_pLogger && this->isWarnEnabled()){
		char buf[LOG_MAX_BUF + 1] = {0};
		va_list ap;
		va_start(ap, fmt);
		std::vsnprintf(buf, LOG_MAX_BUF, fmt, ap);
		LOG4CXX_WARN(m_pLogger, buf);
		va_end(ap);		
	}
}

void Log4CxxLogger::info(const char *fmt, ...){
	if(m_pLogger && this->isInfoEnabled()){
		char buf[LOG_MAX_BUF + 1] = {0};
		va_list ap;
		va_start(ap, fmt);
		std::vsnprintf(buf, LOG_MAX_BUF, fmt, ap);
		LOG4CXX_INFO(m_pLogger, buf);
		va_end(ap);		
	}
}

void Log4CxxLogger::debug(const char *fmt, ...){
	if(m_pLogger && this->isDebugEnabled()){
		char buf[LOG_MAX_BUF + 1] = {0};
		va_list ap;
		va_start(ap, fmt);
		std::vsnprintf(buf, LOG_MAX_BUF, fmt, ap);
		LOG4CXX_DEBUG(m_pLogger, buf);
		va_end(ap);		
	}
}

void Log4CxxLogger::trace(const char *fmt, ...){
	if(m_pLogger && this->isTraceEnabled()){
		char buf[LOG_MAX_BUF + 1] = {0};
		va_list ap;
		va_start(ap, fmt);
		std::vsnprintf(buf, LOG_MAX_BUF, fmt, ap);
		LOG4CXX_TRACE(m_pLogger, buf);
		va_end(ap);		
	}
}

void Log4CxxLogger::error(const char* msg, const std::exception& ex)
{
	if (m_pLogger && isErrorEnabled())
	{
		LOG4CXX_ERROR(m_pLogger, sk::logger::formatException(msg, ex));
	}
}

void Log4CxxLogger::fatal(const char* msg, const std::exception& ex)
{
	if (m_pLogger && isFatalEnabled())
	{
		LOG4CXX_FATAL(m_pLogger, sk::logger::formatException(msg, ex));
	}
}

#endif

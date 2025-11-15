/**
 * @file LoggerFactory.cpp
 * @brief Singleton factory implementation for creating and retrieving logger instances.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 * @date Last modified: November 15, 2025
 */
#include <LoggerFactory.h>
#include <mutex>
#include <string>
#include <map>

#ifdef USE_LOG4CXX
#include <Log4CxxLogger.h>
#define LOGGER(name)  Log4CxxLogger(name)
#elif USE_SPDLOG
#include <SpdlogLogger.h>
#define LOGGER(name)  SpdlogLogger(name)
#else
#include <SimpleLogger.h>
#define LOGGER(name)  SimpleLogger(name)
#endif

using namespace sk::logger;

// static initialization
std::map<std::string, LoggerPtr> LoggerFactory::m_loggers;
std::mutex LoggerFactory::m_factoryLock;

 LoggerFactory& LoggerFactory::getInstance() {
        static LoggerFactory factory;
        return factory;
}

LoggerFactory::~LoggerFactory(void)
{
    m_loggers.clear(); 
}

LoggerPtr LoggerFactory::createLogger(std::string name){
	return  LoggerPtr(new LOGGER(std::move(name)));
}


LoggerPtr LoggerFactory::getLogger(const char* pName){
	std::lock_guard<std::mutex> lock(m_factoryLock);	
	std::string key = std::string(pName);
	std::map<std::string, LoggerPtr>::iterator  it = m_loggers.find(key);

	if(it == m_loggers.end()){
		LoggerPtr pLogger = createLogger(pName);
		m_loggers.insert(std::pair<std::string,LoggerPtr>(key, pLogger));
        return pLogger;
	} else {
        return it->second;
	}
}

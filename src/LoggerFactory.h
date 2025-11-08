/**
 * @file LoggerFactory.h
 * @brief Singleton factory for creating and retrieving logger instances.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 * @date Last modified: November 08, 2025
 */
#ifndef SK_LOGGER_FACTORY_H
#define SK_LOGGER_FACTORY_H

#include <Logger.h>
#include <mutex>
#include <string>
#include <map>

namespace sk { namespace logger {

/**
 * @class LoggerFactory
 * @brief Singleton factory for creating and retrieving logger instances.
 *
 * This class manages logger instances by name and provides a unified
 * interface for obtaining loggers of the selected implementation. It
 * ensures thread-safe access and supports backend flexibility via the
 * logging facade.
 */
class LoggerFactory 
{
public:
	static LoggerFactory& getInstance();
	~LoggerFactory(void);
	static LoggerPtr getLogger(const char* pName);

private:

	static std::map<std::string, LoggerPtr> m_loggers;
	static std::mutex m_factoryLock;

	static LoggerPtr createLogger(std::string name);

};

}}

#endif

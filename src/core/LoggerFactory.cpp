/**
 * @file LoggerFactory.cpp
 * @brief Singleton factory implementation for creating and retrieving logger instances.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 * @date Last modified: November 16, 2025
 */
#include <logger/LoggerFactory.h>
#include "LoggerFactoryImpl.h"
#include "LoggerConfigurator.h"

using namespace sk::logger;

LoggerFactory& LoggerFactory::getInstance() {
    static LoggerFactory factory;
    return factory;
}

LoggerFactory::~LoggerFactory(void) {}

LoggerPtr LoggerFactory::getLogger(const char* pName){
    return LoggerFactoryImpl::getInstance().getLogger(std::string(pName));
}

void LoggerFactory::configureFromJsonString(const std::string& json)
{
    LoggerConfigurator::configureFromJsonString(json);
}

void LoggerFactory::configureFromJsonFile(const std::string& filePath)
{
    LoggerConfigurator::configureFromJsonFile(filePath);
}

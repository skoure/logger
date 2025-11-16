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

#include <logger/Logger.h>

namespace sk { namespace logger {

class LoggerFactory {
public:
    /**
     * @brief Get the singleton LoggerFactory instance.
     */
    static LoggerFactory& getInstance();
    ~LoggerFactory(void);

    /**
     * @brief Get or create a logger by name. Hierarchy-aware.
     * @param pName Logger name (e.g. "App.Database")
     * @return LoggerPtr instance
     */
    static LoggerPtr getLogger(const char* pName);

};

}} // namespace

#endif // SK_LOGGER_FACTORY_H

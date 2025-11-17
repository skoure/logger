/**
 * @file LoggerFactoryImpl.h
 * @brief Internal implementation for LoggerFactory.
 * This allows separation of the LoggerFactory interface and the implementation details.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 16, 2025
 * @date Last modified: November 16, 2025
 *
 */
#ifndef SK_LOGGER_FACTORY_IMPL_H
#define SK_LOGGER_FACTORY_IMPL_H

#include <logger/Logger.h>
#include <LoggerHierarchy.h>
#include <map>
#include <mutex>
#include <string>

namespace sk { namespace logger {

class LoggerFactoryImpl {

public:
    /**
     * @brief Get the singleton LoggerFactory instance.
     */
    static LoggerFactoryImpl& getInstance();

    LoggerPtr getLogger(const std::string& name);

private:
    LoggerFactoryImpl() = default;
    ~LoggerFactoryImpl();

    std::mutex m_factoryLock;
    LoggerHierarchy m_hierarchy;

    static LoggerPtr createLogger(const std::string& name);

    bool applyParentSinks(LoggerPtr childLogger, LoggerPtr parentLogger);
    bool applyParentConfiguration(LoggerPtr childLogger, LoggerPtr parentLogger);

};

}} // namespace

#endif // SK_LOGGER_FACTORY_IMPL_H

/**
 * @file LoggerFactoryImpl.h
 * @brief Internal implementation for LoggerFactory.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 16, 2025
 */
#ifndef SK_LOGGER_FACTORY_IMPL_H
#define SK_LOGGER_FACTORY_IMPL_H

#include <logger/Logger.h>
#include <ILoggerBackend.h>
#include <LoggerHierarchy.h>
#include <SinkConfig.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace sk { namespace logger {

class LoggerFactoryImpl {
public:
    static LoggerFactoryImpl& getInstance();

    /**
     * @brief Set the active backend. Must be called once before getLogger().
     * @param backend Ownership transferred to the factory.
     */
    void setBackend(std::unique_ptr<ILoggerBackend> backend);

    LoggerPtr getLogger(const std::string& name);

    /**
     * @brief Forward a sink configuration request to the active backend.
     *
     * Called by LoggerConfigurator after setLevel() has been applied.
     *
     * @param logger Logger instance to configure.
     * @param sinks  Ordered list of sink descriptors.
     */
    void configureLogger(LoggerPtr logger, const std::vector<SinkConfig>& sinks);

private:
    LoggerFactoryImpl() = default;
    ~LoggerFactoryImpl();

    std::mutex                      m_factoryLock;
    LoggerHierarchy                 m_hierarchy;
    std::unique_ptr<ILoggerBackend> m_backend;

};

}} // namespace sk::logger

#endif // SK_LOGGER_FACTORY_IMPL_H

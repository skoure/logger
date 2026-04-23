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
#include <ostream>
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
     * @brief Creates a real backend logger for a name already in the hierarchy.
     *
     * Called by LazyLogger::append() on first use to perform deferred backend
     * creation. Kept separate from getLogger() to avoid two problems:
     *   1. Circular lookup — the LazyLogger is already in the hierarchy, so
     *      getLogger() would return it again instead of creating a real logger.
     *   2. Re-entrant locking — both methods acquire m_factoryLock.
     *
     * Does not add the result to the hierarchy; the LazyLogger remains the
     * authoritative hierarchy entry and proxies to the returned logger.
     *
     * @param name Logger name (must already exist in the hierarchy as a LazyLogger).
     * @return Real backend LoggerPtr, or nullptr if backend not yet registered.
     */
    LoggerBasePtr createBackendLogger(const std::string& name);

    /**
     * @brief Forward a sink configuration request to the active backend.
     *
     * Called by LoggerConfigurator after setLevel() has been applied.
     *
     * @param logger Logger instance to configure.
     * @param sinks  Ordered list of sink descriptors.
     */
    void configureLogger(LoggerPtr logger, const std::vector<SinkConfig>& sinks);

    /**
     * @brief Forward an ostream sink configuration request to the active backend.
     * @param logger           Logger instance to configure.
     * @param os               Output stream to write to.
     * @param canonicalPattern Log4j-style pattern.
     */
    void configureLoggerWithOstream(LoggerPtr logger, std::ostream& os,
                                    const std::string& canonicalPattern);

private:
    LoggerFactoryImpl() = default;
    ~LoggerFactoryImpl();

    std::mutex                      m_factoryLock;
    LoggerHierarchy                 m_hierarchy;
    std::unique_ptr<ILoggerBackend> m_backend;

};

}} // namespace sk::logger

#endif // SK_LOGGER_FACTORY_IMPL_H

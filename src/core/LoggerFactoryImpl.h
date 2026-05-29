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
#include <ProxyBackend.h>
#include <LoggerHierarchy.h>
#include <SinkConfig.h>
#include <memory>
#include <mutex>
#include <ostream>
#include <set>
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

    /**
     * @brief Forward an ostream sink configuration request to the active backend.
     * @param logger           Logger instance to configure.
     * @param os               Output stream to write to.
     * @param canonicalPattern Log4j-style pattern.
     */
    void configureLoggerWithOstream(LoggerPtr logger, std::ostream& os,
                                    const std::string& canonicalPattern);

    /**
     * @brief Clear the explicitly-set level on every logger so all revert to
     * inherited behaviour before a new configuration is applied.
     */
    void clearAllLevels();

    /**
     * @brief Remove all sinks from every logger in the hierarchy.
     *
     * Called at the start of configure() to establish a clean slate.
     */
    void clearAllSinks();

    /**
     * @brief Clear the flush_on threshold on every logger so all revert to
     * no-flush behaviour before a new configuration is applied.
     */
    void clearAllFlushOn();

    /**
     * @brief Propagate sinks top-down to loggers not listed in @p configured.
     *
     * Walks all loggers in parent-before-child order and calls applyParentSinks()
     * for any logger whose name is not in the configured set.
     *
     * @param configured Set of logger names that received explicit sinks this cycle.
     */
    void propagateInheritedSinks(const std::set<std::string>& configured);

private:
    LoggerFactoryImpl();
    ~LoggerFactoryImpl();

    std::mutex                    m_factoryLock;
    LoggerHierarchy               m_hierarchy;
    std::unique_ptr<ProxyBackend> m_proxyBackend;

};

}} // namespace sk::logger

#endif // SK_LOGGER_FACTORY_IMPL_H

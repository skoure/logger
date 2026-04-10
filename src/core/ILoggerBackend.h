/**
 * @file ILoggerBackend.h
 * @brief Pure abstract interface for logger backend implementations.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 *
 * Each backend (SimpleLogger, spdlog, Log4cxx) implements this interface and
 * is packaged as a separate static library.  LoggerFactoryImpl holds a
 * unique_ptr<ILoggerBackend> and delegates all backend-specific work through it.
 *
 * This header is internal (not installed alongside the public API).
 */
#ifndef SK_ILOGGER_BACKEND_H
#define SK_ILOGGER_BACKEND_H

#include <logger/Logger.h>
#include <SinkConfig.h>
#include <ostream>
#include <string>
#include <vector>

namespace sk { namespace logger {

/**
 * @class ILoggerBackend
 * @brief Abstract interface that every logger backend must implement.
 */
class ILoggerBackend {
public:
    virtual ~ILoggerBackend() = default;

    /**
     * @brief Create a new logger instance for the given name.
     * @param name Logger name (e.g. "App.Database")
     * @return Newly constructed logger
     */
    virtual LoggerPtr createLogger(const std::string& name) = 0;

    /**
     * @brief Copy or link the parent's appenders/sinks into the child.
     *
     * Called by LoggerFactoryImpl after a new child logger is created.
     * Backends that manage their own hierarchy (e.g. Log4cxx) may leave
     * this as a no-op and return early via supportsNativeHierarchy().
     *
     * @param child  Newly created child logger
     * @param parent Parent logger whose sinks should be inherited
     */
    virtual void applyParentSinks(LoggerPtr child, LoggerPtr parent) = 0;

    /**
     * @brief Returns true if the backend handles hierarchy internally.
     *
     * When true, LoggerFactoryImpl skips the parent-configuration step
     * entirely (level copy + applyParentSinks) because the backend's own
     * framework already manages those relationships.
     * Log4cxx returns true; SimpleLogger and spdlog return false.
     */
    virtual bool supportsNativeHierarchy() const = 0;

    /**
     * @brief Configure a logger's sinks from the provided SinkConfig list.
     *
     * Called by LoggerConfigurator after it has set the logger's level.
     * The default implementation is a no-op so that backends that do not
     * yet support programmatic sink configuration still compile and work.
     *
     * @param logger Logger instance to configure.
     * @param sinks  Ordered list of sink descriptors.
     */
    virtual void configureLogger(LoggerPtr /*logger*/,
                                 const std::vector<SinkConfig>& /*sinks*/) {}

    /**
     * @brief Configure a logger to write to an arbitrary std::ostream.
     *
     * The caller must ensure @p os outlives the logger.  The default
     * implementation is a no-op so that backends which do not yet provide
     * this capability still compile and link correctly.
     *
     * @param logger           Logger instance to configure.
     * @param os               Output stream to write to.
     * @param canonicalPattern Log4j-style pattern (e.g. "[%p] %m%n").
     */
    virtual void configureLoggerWithOstream(LoggerPtr /*logger*/,
                                            std::ostream& /*os*/,
                                            const std::string& /*canonicalPattern*/) {}
};

}} // namespace sk::logger

#endif // SK_ILOGGER_BACKEND_H

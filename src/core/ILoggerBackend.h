/**
 * @file ILoggerBackend.h
 * @brief Base interface for logger backend implementations.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 *
 * Each backend implements ILoggerBackend and is packaged as a separate static
 * library.  LoggerFactoryImpl holds a unique_ptr<ILoggerBackend> and delegates
 * all backend-specific work through it.
 *
 * Backends where the factory is responsible for propagating sink inheritance
 * additionally implement IManagedSinkBackend (see IManagedSinkBackend.h).
 *
 * This header is internal (not installed alongside the public API).
 */
#ifndef SK_ILOGGER_BACKEND_H
#define SK_ILOGGER_BACKEND_H

#include <logger/Logger.h>
#include <LoggerBase.h>
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
     * @return Newly constructed logger (always a LoggerBase subclass)
     */
    virtual LoggerBasePtr createLogger(const std::string& name) = 0;

    /**
     * @brief Configure a logger's sinks from the provided SinkConfig list.
     *
     * Called by LoggerConfigurator after it has set the logger's level.
     * The default implementation is a no-op; backends that do not support
     * programmatic sink configuration do not need to override it.
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
     * implementation is a no-op; backends that do not support ostream
     * output do not need to override it.
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

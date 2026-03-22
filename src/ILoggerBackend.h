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
#include <string>

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
};

}} // namespace sk::logger

#endif // SK_ILOGGER_BACKEND_H

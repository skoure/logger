/**
 * @file IManagedSinkBackend.h
 * @brief Extended interface for backends where the factory manages sink inheritance.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: April 23, 2026
 *
 * Implement this interface in addition to ILoggerBackend when the factory is
 * responsible for propagating sink inheritance at logger creation time and for
 * clearing sinks during reconfiguration.  Backends that manage appender
 * inheritance through their own internal registry do not need to implement this.
 *
 * This header is internal (not installed alongside the public API).
 */
#ifndef SK_IMANAGED_SINK_BACKEND_H
#define SK_IMANAGED_SINK_BACKEND_H

#include <ILoggerBackend.h>

namespace sk { namespace logger {

/**
 * @class IManagedSinkBackend
 * @brief Extended interface for backends where LoggerFactoryImpl manages sink inheritance.
 *
 * Implement this when the factory should call applyParentSinks() at logger
 * creation time and clearSinks() at the start of each configure() call.
 * Backends that manage their own appender hierarchy do not need to implement this.
 */
class IManagedSinkBackend : public ILoggerBackend {
public:
    /**
     * @brief Copy or link the parent's sinks into the child.
     *
     * Called after a new child logger is created so it inherits the parent's
     * current sink configuration.
     *
     * @param child  Newly created child logger.
     * @param parent Parent logger whose sinks should be inherited.
     */
    virtual void applyParentSinks(LoggerPtr child, LoggerPtr parent) = 0;

    /**
     * @brief Remove all sinks from the given logger.
     *
     * Called at the start of each configure() call to establish a clean slate
     * before the new configuration is applied.
     *
     * @param logger Logger whose sinks should be cleared.
     */
    virtual void clearSinks(LoggerPtr logger) = 0;
};

}} // namespace sk::logger

#endif // SK_IMANAGED_SINK_BACKEND_H

/**
 * @file SpdlogBackend.h
 * @brief ILoggerBackend implementation for the spdlog backend.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#ifndef SK_SPDLOG_BACKEND_H
#define SK_SPDLOG_BACKEND_H

#include <IManagedSinkBackend.h>

namespace sk { namespace logger {

class SpdlogBackend : public IManagedSinkBackend {
public:
    LoggerBasePtr createLogger(const std::string& name) override;
    void      applyParentSinks(LoggerPtr child, LoggerPtr parent) override;
    void      configureLogger(LoggerPtr logger,
                              const std::vector<SinkConfig>& sinks) override;

    void clearSinks(LoggerPtr logger) override;
};

/**
 * @brief Wire the spdlog backend into LoggerFactory.
 *
 * Call once at program startup before any LoggerFactory::getLogger() call:
 * @code
 *   sk::logger::useSpdlogBackend();
 * @endcode
 */
void useSpdlogBackend();

}} // namespace sk::logger

#endif // SK_SPDLOG_BACKEND_H

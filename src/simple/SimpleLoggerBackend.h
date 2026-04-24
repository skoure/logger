/**
 * @file SimpleLoggerBackend.h
 * @brief ILoggerBackend implementation for the SimpleLogger backend.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#ifndef SK_SIMPLE_LOGGER_BACKEND_H
#define SK_SIMPLE_LOGGER_BACKEND_H

#include <IManagedSinkBackend.h>

namespace sk { namespace logger {

class SimpleLoggerBackend : public IManagedSinkBackend {
public:
    LoggerBasePtr createLogger(const std::string& name) override;
    void      applyParentSinks(LoggerPtr child, LoggerPtr parent) override;
    void      configureLogger(LoggerPtr logger,
                              const std::vector<SinkConfig>& sinks) override;
    void      configureLoggerWithOstream(LoggerPtr logger, std::ostream& os,
                                         const std::string& canonicalPattern) override;
    void      clearSinks(LoggerPtr logger) override;
};

/**
 * @brief Wire the SimpleLogger backend into LoggerFactory.
 *
 * Call once at program startup before any LoggerFactory::getLogger() call:
 * @code
 *   sk::logger::useSimpleLoggerBackend();
 * @endcode
 */
void useSimpleLoggerBackend();

}} // namespace sk::logger

#endif // SK_SIMPLE_LOGGER_BACKEND_H

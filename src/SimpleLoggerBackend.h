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

#include <ILoggerBackend.h>

namespace sk { namespace logger {

class SimpleLoggerBackend : public ILoggerBackend {
public:
    LoggerPtr createLogger(const std::string& name) override;
    void      applyParentSinks(LoggerPtr child, LoggerPtr parent) override;
    bool      supportsNativeHierarchy() const override { return false; }
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

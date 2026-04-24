/**
 * @file Log4CxxBackend.h
 * @brief ILoggerBackend implementation for the Log4cxx backend.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#ifndef SK_LOG4CXX_BACKEND_H
#define SK_LOG4CXX_BACKEND_H

#include <ILoggerBackend.h>

namespace sk { namespace logger {

class Log4CxxBackend : public ILoggerBackend {
public:
    LoggerBasePtr createLogger(const std::string& name) override;
    void configureLogger(LoggerPtr logger,
                         const std::vector<SinkConfig>& sinks) override;
    void configureLoggerWithOstream(LoggerPtr logger, std::ostream& os,
                                    const std::string& canonicalPattern) override;
};

/**
 * @brief Wire the Log4cxx backend into LoggerFactory.
 *
 * Call once at program startup before any LoggerFactory::getLogger() call:
 * @code
 *   sk::logger::useLog4CxxBackend();
 * @endcode
 */
void useLog4CxxBackend();

}} // namespace sk::logger

#endif // SK_LOG4CXX_BACKEND_H

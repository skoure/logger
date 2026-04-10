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

#include <ILoggerBackend.h>
#include <ostream>

namespace sk { namespace logger {

class SpdlogBackend : public ILoggerBackend {
public:
    LoggerPtr createLogger(const std::string& name) override;
    void      applyParentSinks(LoggerPtr child, LoggerPtr parent) override;
    bool      supportsNativeHierarchy() const override { return false; }
    void      configureLogger(LoggerPtr logger,
                              const std::vector<SinkConfig>& sinks) override;

    /**
     * @brief Configure a logger to write to an arbitrary std::ostream.
     *
     * Translates @p canonicalPattern and attaches an ostream_sink_mt with the
     * custom flag formatters (marker, level name, thread name) already applied.
     * Useful in unit tests where stdout capture via _dup2 is unreliable on
     * Windows (the platform's color sink writes through the Windows Console
     * HANDLE, not the C-runtime FILE).
     */
    void configureLoggerWithOstream(LoggerPtr logger, std::ostream& os,
                                    const std::string& canonicalPattern) override;
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

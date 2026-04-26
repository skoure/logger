/**
 * @file LoggerUtils.h
 * @brief Internal utility functions for logger implementations.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 20, 2026
 */
#ifndef SK_LOGGER_UTILS_H
#define SK_LOGGER_UTILS_H

#include <exception>
#include <string>
#include <thread>

namespace sk { namespace logger {

#ifdef _WIN32
    constexpr const char* eol = "\r\n";
#else
    constexpr const char* eol = "\n";
#endif

/**
 * @brief Formats an exception into a human-readable string containing
 *        the exception type, message, and an optional stacktrace.
 *
 * The stacktrace reflects the call stack at the point this function is
 * invoked (typically inside a catch block), not the original throw site.
 * Meaningful frame names require the binary to be compiled with debug
 * symbols (-g or RelWithDebInfo build type).
 *
 * @param msg Context message prepended to the output (e.g. "Failed to open file").
 * @param ex  The exception to format.
 * @return    Formatted string: "<msg>: <type>: <what()>\nStacktrace:\n<frames>"
 */
std::string formatException(const char* msg, const std::exception& ex);

/**
 * @brief Returns the OS-level name of the calling thread.
 *
 * On Linux this uses pthread_getname_np().  On platforms where thread naming
 * is not available, returns an empty string.
 *
 * @return Thread name string, or empty string if unavailable.
 */
std::string getCurrentThreadName();

}} // namespace sk::logger

#endif // SK_LOGGER_UTILS_H

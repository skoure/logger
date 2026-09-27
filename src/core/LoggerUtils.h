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
#include <vector>

#include <logger/Marker.h>

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
 * @param skip Number of top stack frames to omit from generated trace.
 * @return    Formatted string: "<msg>: <type>: <what()>\nStacktrace:\n<frames>"
 */
std::string formatException(const char* msg, const std::exception& ex, int skip = 0);

/**
 * @brief Returns the OS-level name of the calling thread.
 *
 * On Linux this uses pthread_getname_np().  On platforms where thread naming
 * is not available, returns an empty string.
 *
 * @return Thread name string, or empty string if unavailable.
 */
std::string getCurrentThreadName();

/**
 * @brief Joins marker names into a single string with a custom separator.
 * 
 * Renders nothing for an empty list, the bare name for one marker, and
 * "A<separator>B<separator>C" for several.  Used by every backend wherever the marker names are rendered or stored as a single string (e.g. %M pattern output, log4cxx MDC).
 * @param markers Markers to join (nullptr entries are skipped).
 * @param separator Separator string to insert between names (default: ", ").
 * @return Marker names separated by the specified separator, or an empty string.
 */
 std::string joinMarkerNames(const std::vector<const Marker*>& markers, const char* separator = ", ");

}} // namespace sk::logger

#endif // SK_LOGGER_UTILS_H

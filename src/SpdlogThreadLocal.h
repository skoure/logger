/**
 * @file SpdlogThreadLocal.h
 * @brief Thread-local bridge for passing LogRecord metadata to spdlog formatters.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 *
 * Spdlog's custom formatter API only receives spdlog::details::log_msg, which
 * has no knowledge of our LogRecord.  SpdlogLogger::append() sets these
 * thread-locals before calling spdlog, then clears them after.  The custom
 * formatter classes read from here at format time.
 */
#ifndef SK_SPDLOG_THREAD_LOCAL_H
#define SK_SPDLOG_THREAD_LOCAL_H

#include <string>

namespace sk { namespace logger { namespace spdlog_tls {

/** Name of the active marker, or nullptr when no marker is set. */
extern thread_local const char* markerName;

/** Name of the logging thread (empty string if not set). */
extern thread_local std::string threadName;

}}} // namespace sk::logger::spdlog_tls

#endif // SK_SPDLOG_THREAD_LOCAL_H

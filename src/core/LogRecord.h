/**
 * @file LogRecord.h
 * @brief Captures all metadata for a single log event.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#ifndef SK_LOG_RECORD_H
#define SK_LOG_RECORD_H

#include <chrono>
#include <string>
#include <thread>
#include <logger/Logger.h>
#include <logger/Marker.h>

namespace sk { namespace logger {

/**
 * @struct LogRecord
 * @brief Carries all metadata for a single log event passed to append().
 *
 * Constructed once per log call by LoggerBase and delivered to the backend's
 * append() method. The message is already formatted before the record is
 * created. Adding new fields here (e.g. Marker, thread name) extends the
 * pipeline without touching the public Logger interface or any backend.
 */
struct LogRecord
{
    Logger::Level                         level;
    std::string                           loggerName;
    std::string                           message;
    std::chrono::system_clock::time_point timestamp;
    const Marker*                         marker     = nullptr;
    std::thread::id                       threadId;
    std::string                           threadName;
};

}} // namespace sk::logger

#endif // SK_LOG_RECORD_H

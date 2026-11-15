/**
 * @file LogRecordPattern.h
 * @brief Renders canonical patterns using a LogRecord.
 * 
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 18, 2026
 */
#ifndef SK_LOG_RECORD_PATTERN_H
#define SK_LOG_RECORD_PATTERN_H

#include "CompiledPattern.h"

#include <LogRecord.h>

namespace sk { namespace logger {

class LogRecordPattern {
public:
    static std::string render(const CompiledPattern& pattern, const LogRecord& record);

private:
    LogRecordPattern() = delete;
    ~LogRecordPattern() = delete;
};

}} // namespace sk::logger

#endif // SK_LOG_RECORD_PATTERN_H
/**
 * @file GlobalOstreamSink.h
 * @brief Internal registration and dispatch for global ostream sinks.
 * 
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 18, 2026
 */
#ifndef SK_GLOBAL_OSTREAM_SINK_H
#define SK_GLOBAL_OSTREAM_SINK_H

#include <LogRecord.h>
#include <logger/Filter.h>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace sk { namespace logger {

void addGlobalOstreamSink(std::ostream& stream, std::string pattern,
                          std::vector<std::shared_ptr<const Filter>> filters,
                          Logger::Level level);
void clearGlobalOstreamSinks();
void dispatchGlobalOstreamSinks(const LogRecord& record,
                               std::optional<Logger::Level> flushOn);

}} // namespace sk::logger

#endif // SK_GLOBAL_OSTREAM_SINK_H
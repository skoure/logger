/**
 * @file BuiltinFilterFactory.h
 * @brief Internal constructors for built-in filter implementations.
 * 
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 18, 2026
 * 
 */
#ifndef SK_BUILTIN_FILTER_FACTORY_H
#define SK_BUILTIN_FILTER_FACTORY_H

#include <logger/LoggerNameFilter.h>
#include <logger/MarkerFilter.h>
#include <logger/RegexFilter.h>
#include <memory>
#include <string>
#include <vector>

namespace sk { namespace logger {

std::shared_ptr<MarkerFilter> makeMarkerFilter(
    std::vector<std::shared_ptr<const Marker>> markers, FilterMode mode);

std::shared_ptr<RegexFilter> makeRegexFilter(std::string pattern, FilterMode mode);

std::shared_ptr<LoggerNameFilter> makeLoggerNameFilter(
    std::vector<std::string> names, FilterMode mode);

}} // namespace sk::logger

#endif // SK_BUILTIN_FILTER_FACTORY_H
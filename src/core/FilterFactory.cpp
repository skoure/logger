/**
 * @file FilterFactory.cpp
 * @brief Factory for constructing built-in filters.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 18, 2026
 */
#include <logger/FilterFactory.h>

#include "BuiltinFilterFactory.h"

#include <stdexcept>
#include <utility>

using namespace sk::logger;

namespace {

void validateMode(FilterMode mode)
{
    if (mode != FilterMode::Include && mode != FilterMode::Exclude)
        throw std::invalid_argument("Filter mode must be Include or Exclude");
}

} // namespace

std::shared_ptr<MarkerFilter> FilterFactory::createMarkerFilter(
    std::vector<std::shared_ptr<const Marker>> markers, FilterMode mode)
{
    validateMode(mode);
    return makeMarkerFilter(std::move(markers), mode);
}

std::shared_ptr<RegexFilter> FilterFactory::createRegexFilter(
    std::string pattern, FilterMode mode)
{
    validateMode(mode);
    return makeRegexFilter(std::move(pattern), mode);
}

std::shared_ptr<LoggerNameFilter> FilterFactory::createLoggerNameFilter(
    std::vector<std::string> names, FilterMode mode)
{
    validateMode(mode);
    return makeLoggerNameFilter(std::move(names), mode);
}
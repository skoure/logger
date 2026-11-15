/**
 * @file MarkerPatternLayout.cpp
 * @brief Builds log4cxx layouts with per-token marker converters.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 14, 2026
 */
#include "MarkerPatternLayout.h"

#include "MarkerPatternConverter.h"

#include <stdexcept>
#include <string>
#include <utility>

using namespace sk::logger;

MarkerPatternLayout::MarkerPatternLayout(
    const log4cxx::LogString& pattern,
    std::vector<MarkerPatternOptions> markerOptions)
    : log4cxx::PatternLayout(), markerOptions_(std::move(markerOptions))
{
    setConversionPattern(pattern);
}

log4cxx::pattern::PatternMap MarkerPatternLayout::getFormatSpecifiers()
{
    log4cxx::pattern::PatternMap specifiers =
        log4cxx::PatternLayout::getFormatSpecifiers();
    const std::vector<MarkerPatternOptions> options = markerOptions_;
    specifiers[LOG4CXX_STR("K")] = [options](
        const std::vector<log4cxx::LogString>& converterOptions) {
        if (converterOptions.size() != 1)
            throw std::runtime_error("Log4Cxx marker converter requires an index");
        const std::size_t index = static_cast<std::size_t>(std::stoul(
            std::string(converterOptions.front().begin(),
                        converterOptions.front().end())));
        if (index >= options.size())
            throw std::runtime_error("Log4Cxx marker converter index is out of range");
        return std::make_shared<MarkerPatternConverter>(options[index]);
    };
    return specifiers;
}
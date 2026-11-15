/**
 * @file MarkerPatternConverter.cpp
 * @brief Formats configured marker tokens in log4cxx layout
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 14, 2026
 */
#include "MarkerPatternConverter.h"

#include <string>
#include <utility>
#include <vector>

using namespace sk::logger;

MarkerPatternConverter::MarkerPatternConverter(MarkerPatternOptions options)
    : LoggingEventPatternConverter(LOG4CXX_STR("marker"),
                                   LOG4CXX_STR("marker")),
      options_(std::move(options))
{
}

const log4cxx::helpers::Class& MarkerPatternConverter::getClass() const
{
    return LoggingEventPatternConverter::getStaticClass();
}

const void* MarkerPatternConverter::cast(
    const log4cxx::helpers::Class& clazz) const
{
    if (&clazz == &log4cxx::helpers::Object::getStaticClass())
        return static_cast<const log4cxx::helpers::Object*>(this);
    return LoggingEventPatternConverter::cast(clazz);
}

bool MarkerPatternConverter::instanceof(
    const log4cxx::helpers::Class& clazz) const
{
    return cast(clazz) != nullptr;
}

void MarkerPatternConverter::format(
    const log4cxx::spi::LoggingEventPtr& event,
    log4cxx::LogString& output,
    log4cxx::helpers::Pool&) const
{
    log4cxx::LogString countText;
    if (!event->getMDC(LOG4CXX_STR("__sk_logger_marker_count"), countText))
        return;

    const std::size_t count = static_cast<std::size_t>(
        std::stoul(std::string(countText.begin(), countText.end())));
    std::vector<std::string> markers;
    markers.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
    {
        const std::string key = "__sk_logger_marker_" + std::to_string(i);
        log4cxx::LogString marker;
        if (event->getMDC(log4cxx::LogString(key.begin(), key.end()), marker))
            markers.emplace_back(marker.begin(), marker.end());
    }
    append(output, renderMarkerNames(markers, options_));
}
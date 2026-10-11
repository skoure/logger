/**
 * @file MarkerFilterConfig.h
 * @brief Internal marker filter data and matching behavior.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 * 
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 11, 2026
 */
#ifndef SK_MARKER_FILTER_CONFIG_H
#define SK_MARKER_FILTER_CONFIG_H

#include <logger/Marker.h>
#include <logger/MarkerFilter.h>
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace sk { namespace logger {

struct MarkerFilterConfig
{
    std::vector<std::string> names;
    bool exclude = false;

    MarkerFilterConfig() = default;

    explicit MarkerFilterConfig(const MarkerFilter& filter)
        : names(filter.getNames()),
          exclude(filter.getMode() == MarkerFilter::Mode::Exclude)
    {}

    bool empty() const { return names.empty(); }

    bool matches(std::string_view name) const
    {
        return std::find_if(names.begin(), names.end(), [name](const std::string& filterName) {
            return std::string_view(filterName) == name;
        }) != names.end();
    }

    bool passesAny(const std::vector<const Marker*>& recordMarkers) const
    {
        if (names.empty()) return true;
        if (recordMarkers.empty()) return exclude;

        const bool hasMatching = std::any_of(recordMarkers.begin(), recordMarkers.end(),
            [this](const Marker* marker) {
                return marker && matches(marker->getName());
            });
        return exclude ? !hasMatching : hasMatching;
    }
};

}} // namespace sk::logger

#endif // SK_MARKER_FILTER_CONFIG_H
/**
 * @file MarkerFilter.cpp
 * @brief Internal marker filter implementation.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 18, 2026
 */
#include "BuiltinFilterFactory.h"

#include <LogRecord.h>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace sk { namespace logger {
namespace {

class MarkerFilterImpl final : public MarkerFilter {
public:
    const std::vector<std::shared_ptr<const Marker>>& getMarkers() const override
    {
        return markers_;
    }

    void setMarkers(std::vector<std::shared_ptr<const Marker>> markers) override
    {
        if (std::any_of(markers.begin(), markers.end(), [](const auto& marker) {
                return !marker;
            }))
            throw std::invalid_argument("MarkerFilter cannot contain null markers");
        markers_ = std::move(markers);
    }

    Mode getMode() const override { return mode_; }
    void setMode(Mode mode) override { mode_ = mode; }

    bool accepts(const LogRecord& record) const override
    {
        if (markers_.empty())
            return true;

        const bool hasMatch = std::any_of(
            record.markers.begin(), record.markers.end(), [this](const Marker* eventMarker) {
                return eventMarker && std::any_of(
                    markers_.begin(), markers_.end(), [eventMarker](const auto& filterMarker) {
                        return filterMarker->getName() == eventMarker->getName();
                    });
            });
        return mode_ == Mode::Exclude ? !hasMatch : hasMatch;
    }

    std::vector<std::shared_ptr<const Marker>> markers_;
    Mode mode_ = Mode::Include;
};

} // namespace

std::shared_ptr<MarkerFilter> makeMarkerFilter(
    std::vector<std::shared_ptr<const Marker>> markers, FilterMode mode)
{
    auto filter = std::make_shared<MarkerFilterImpl>();
    filter->setMarkers(std::move(markers));
    filter->setMode(mode);
    return filter;
}

}} // namespace sk::logger
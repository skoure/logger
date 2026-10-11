/**
 * @file MarkerFactory.cpp
 * @brief Marker Factory Implementation
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 27, 2025
 */
#include <logger/MarkerFactory.h>
#include <logger/Marker.h>
#include <algorithm>
#include <unordered_map>
#include <mutex>
#include <stdexcept>

namespace sk { namespace logger {

/**
 * @class Marker
 * @brief The concrete implementation of Marker.
 *
 * This class is hidden from the public API and only known internally.
 */
class MarkerImpl final : public Marker {
public:
    // Constructor is private/protected in a factory pattern, or just kept internal
    explicit MarkerImpl(const std::string& name) : name_(name) {}

    const std::string& getName() const override { return name_; }

private:
    std::string name_;
};

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

private:
    std::vector<std::shared_ptr<const Marker>> markers_;
    Mode mode_ = Mode::Include;
};

// Global registry to store unique markers.
// Using a shared_ptr for thread-safe memory management.
using MarkerRegistry = std::unordered_map<std::string, std::shared_ptr<Marker>>;

namespace {
MarkerRegistry& markerRegistry() {
    static MarkerRegistry registry;
    return registry;
}

std::mutex& registryMutex() {
    static std::mutex mutex;
    return mutex;
}
} // namespace

std::shared_ptr<Marker> MarkerFactory::getMarker(const std::string& name) {
    // Acquire lock to ensure thread-safe access to the registry
    std::lock_guard<std::mutex> lock(registryMutex());

    // 1. Check if the marker already exists
    auto& registry = markerRegistry();
    auto it = registry.find(name);
    if (it != registry.end()) {
        // Exists: return the existing shared pointer
        return it->second;
    }

    // 2. Doesn't exist: create a new marker
    // Note: We use std::make_shared to manage the lifetime
    std::shared_ptr<Marker> newMarker = std::make_shared<MarkerImpl>(name);

    // 3. Register the new marker
    registry[name] = newMarker;
    
    // 4. Return the new marker
    return newMarker;
}

std::shared_ptr<MarkerFilter> MarkerFactory::createFilter()
{
    return std::make_shared<MarkerFilterImpl>();
}

}} // namespace sk::logger
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
#include <unordered_map>
#include <mutex>

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

// Global registry to store unique markers.
// Using a shared_ptr for thread-safe memory management.
using MarkerRegistry = std::unordered_map<std::string, std::shared_ptr<Marker>>;

// Static members for the singleton registry and its access protection
static MarkerRegistry g_markerRegistry;
static std::mutex g_registryMutex;

std::shared_ptr<Marker> MarkerFactory::getMarker(const std::string& name) {
    // Acquire lock to ensure thread-safe access to the registry
    std::lock_guard<std::mutex> lock(g_registryMutex);

    // 1. Check if the marker already exists
    auto it = g_markerRegistry.find(name);
    if (it != g_markerRegistry.end()) {
        // Exists: return the existing shared pointer
        return it->second;
    }

    // 2. Doesn't exist: create a new marker
    // Note: We use std::make_shared to manage the lifetime
    std::shared_ptr<Marker> newMarker = std::make_shared<MarkerImpl>(name);

    // 3. Register the new marker
    g_markerRegistry[name] = newMarker;
    
    // 4. Return the new marker
    return newMarker;
}

}} // namespace sk::logger
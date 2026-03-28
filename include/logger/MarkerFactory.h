/**
 * @file MarkerFactory.h
 * @brief Singleton factory for creating and retrieving unique Marker instances.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 27, 2025
 */
#ifndef SK_LOGGER_MARKERFACTORY_H
#define SK_LOGGER_MARKERFACTORY_H

#include "Marker.h"
#include <memory>
#include <string>

namespace sk { namespace logger {

/**
 * @class MarkerFactory
 * @brief Singleton factory for creating and retrieving unique IMarker instances.
 *
 * This is the public API entry point for obtaining a marker.
 */
class MarkerFactory {
public:
    /**
     * @brief Retrieves a unique marker instance by name.
     * If a marker with this name exists, the existing instance is returned.
     * Otherwise, a new instance is created and registered.
     * @param name The unique name of the marker.
     * @return A shared pointer to the unique Marker instance.
     */
    static std::shared_ptr<Marker> getMarker(const std::string& name);

private:
    // Prevent instantiation (all methods are static)
    MarkerFactory() = delete;
    ~MarkerFactory() = delete;
};

}} // namespace sk::logger

#endif // SK_LOGGER_MARKERFACTORY_H
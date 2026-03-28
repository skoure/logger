/**
 * @file Marker.h
 * @brief Public interface for a logging marker.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 27, 2025
 */
#ifndef SK_LOGGER_MARKER_H
#define SK_LOGGER_MARKER_H

#include <string>
#include <memory>
#include <ostream>

namespace sk { namespace logger {

/**
 * @class Marker
 * @brief Public interface for a logging marker.
 *
 * This class serves as the abstract base class (interface) for all Markers.
 * Users only interact with shared_ptr<Marker> objects.
 */
class Marker {
public:
    virtual ~Marker() = default;

    /**
     * @brief Get the marker's name.
     */
    virtual const std::string& getName() const = 0;

    // Overload the stream insertion operator (implemented in a non-member function)
    friend std::ostream& operator<<(std::ostream& os, const Marker& marker);
};

// Non-member stream operator for the interface
inline std::ostream& operator<<(std::ostream& os, const Marker& marker) {
    os << marker.getName();
    return os;
}

// Forward declaration of the factory class
class MarkerFactory;

}} // namespace sk::logger

#endif // SK_LOGGER_MARKER_H
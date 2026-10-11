/**
 * @file MarkerFilter.h
 * @brief Public interface for configuring marker-based sink filtering.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 * 
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 11, 2026
 */
#ifndef SK_LOGGER_MARKER_FILTER_H
#define SK_LOGGER_MARKER_FILTER_H

#include <string>
#include <vector>

namespace sk { namespace logger {

class MarkerFilter {
public:
    enum class Mode {
        Include,
        Exclude
    };

    virtual ~MarkerFilter() = default;

    virtual const std::vector<std::string>& getNames() const = 0;
    virtual void setNames(std::vector<std::string> names) = 0;
    virtual Mode getMode() const = 0;
    virtual void setMode(Mode mode) = 0;
};

}} // namespace sk::logger

#endif // SK_LOGGER_MARKER_FILTER_H
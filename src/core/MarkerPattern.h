 /**
 * @file MarkerPattern.h
 * @brief Parses and renders marker options in canonical patterns.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 14, 2026
 */
#ifndef SK_MARKER_PATTERN_H
#define SK_MARKER_PATTERN_H

#include <logger/Marker.h>
#include <string>
#include <vector>

namespace sk { namespace logger {

/**
 * @brief Parsed mode, marker names, and separator for a `%M{...}` token.
 */
struct MarkerPatternOptions
{
    enum class Mode { All, Include, Exclude };

    Mode mode = Mode::All;
    std::vector<std::string> names;
    std::string separator = ", ";
};

/**
 * @brief Parses marker options following a `%M` token.
 * @param pattern Canonical pattern containing the token.
 * @param index In/out position; advances past the option body when present.
 * @return Parsed options, or defaults when no option body follows `%M`.
 * @throws PatternLayoutParseException if the option body is malformed.
 */
MarkerPatternOptions parseMarkerPatternOptions(const std::string& pattern,
                                               std::size_t& index);

/**
 * @brief Validates marker option bodies in top-level pattern tokens.
 * @param pattern Canonical pattern to validate; date-format contents are ignored.
 * @throws PatternLayoutParseException if a marker option body is malformed.
 */
void validateMarkerPatternOptions(const std::string& pattern);

/**
 * @brief Filters and joins marker names while skipping null marker pointers.
 * @param markers Marker pointers attached to the event.
 * @param options Selection and separator options for the pattern token.
 * @return Filtered marker names joined with the configured separator.
 */
std::string renderMarkerNames(const std::vector<const Marker*>& markers,
                              const MarkerPatternOptions& options);

/**
 * @brief Filters and joins marker names.
 * @param markers Marker names attached to the event.
 * @param options Selection and separator options for the pattern token.
 * @return Filtered marker names joined with the configured separator.
 */
std::string renderMarkerNames(const std::vector<std::string>& markers,
                              const MarkerPatternOptions& options);

}} // namespace sk::logger

#endif // SK_MARKER_PATTERN_H
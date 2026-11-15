 /**
 * @file MarkerPatternLayout.h
 * @brief PatternLayout with support for configured marker tokens.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 14, 2026
 */
#ifndef SK_LOG4CXX_MARKER_PATTERN_LAYOUT_H
#define SK_LOG4CXX_MARKER_PATTERN_LAYOUT_H

#include <MarkerPattern.h>
#include <log4cxx/patternlayout.h>

namespace sk { namespace logger {

/** @brief Registers a converter for each configured marker pattern token. */
class MarkerPatternLayout final : public log4cxx::PatternLayout
{
public:
    /** @brief Initializes a layout with translated syntax and per-token rules. */
    MarkerPatternLayout(const log4cxx::LogString& pattern,
                        std::vector<MarkerPatternOptions> markerOptions);

protected:
    /** @brief Adds the custom marker converter to native log4cxx specifiers. */
    log4cxx::pattern::PatternMap getFormatSpecifiers() override;

private:
    std::vector<MarkerPatternOptions> markerOptions_;
};

}} // namespace sk::logger

#endif // SK_LOG4CXX_MARKER_PATTERN_LAYOUT_H
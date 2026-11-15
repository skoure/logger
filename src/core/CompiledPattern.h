/**
 * @file CompiledPattern.h
 * @brief Parsed canonical pattern data for repeated log-record rendering.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 15, 2026
 */
#ifndef SK_COMPILED_PATTERN_H
#define SK_COMPILED_PATTERN_H

#include "MarkerPattern.h"

#include <cstddef>
#include <string>
#include <vector>

namespace sk { namespace logger {

class CompiledPattern
{
public:
    struct MarkerToken
    {
        MarkerPatternOptions options;
        std::size_t endIndex;
    };

    explicit CompiledPattern(std::string pattern);

    const std::string& getPattern() const { return pattern_; }
    const std::vector<MarkerToken>& markerTokens() const { return markerTokens_; }

private:
    std::string pattern_;
    std::vector<MarkerToken> markerTokens_;
};

}} // namespace sk::logger

#endif // SK_COMPILED_PATTERN_H
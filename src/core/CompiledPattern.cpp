/**
 * @file CompiledPattern.cpp
 * @brief Compiles canonical patterns and caches marker token options.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 15, 2026
 */
#include "CompiledPattern.h"

#include <cctype>
#include <utility>

using namespace sk::logger;

namespace {

std::size_t skipModifier(const std::string& pattern, std::size_t index)
{
    if (index < pattern.size() && pattern[index] == '-')
        ++index;
    while (index < pattern.size()
           && std::isdigit(static_cast<unsigned char>(pattern[index])))
        ++index;
    if (index < pattern.size() && pattern[index] == '.')
    {
        ++index;
        while (index < pattern.size()
               && std::isdigit(static_cast<unsigned char>(pattern[index])))
            ++index;
    }
    return index;
}

std::size_t skipDateFormat(const std::string& pattern, std::size_t index)
{
    if (index >= pattern.size() || pattern[index] != '{')
        return index;
    const std::size_t close = pattern.find('}', index + 1);
    return close == std::string::npos ? index : close + 1;
}

} // namespace

CompiledPattern::CompiledPattern(std::string pattern)
    : pattern_(std::move(pattern))
{
    std::size_t index = 0;
    while (index < pattern_.size())
    {
        if (pattern_[index] != '%')
        {
            ++index;
            continue;
        }

        ++index;
        if (index >= pattern_.size())
            continue;
        if (pattern_[index] == '%')
        {
            ++index;
            continue;
        }

        index = skipModifier(pattern_, index);
        if (index >= pattern_.size())
            continue;

        const char token = pattern_[index++];
        if (token == 'd')
            index = skipDateFormat(pattern_, index);
        else if (token == 'M')
        {
            MarkerPatternOptions options = parseMarkerPatternOptions(pattern_, index);
            markerTokens_.push_back({std::move(options), index});
        }
    }
}
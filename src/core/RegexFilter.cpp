/**
 * @file RegexFilter.cpp
 * @brief Internal message regex filter implementation.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 18, 2026
 */
#include "BuiltinFilterFactory.h"

#include <LogRecord.h>
#include <regex>
#include <utility>

namespace sk { namespace logger {
namespace {

class RegexFilterImpl final : public RegexFilter {
public:
    RegexFilterImpl(std::string pattern, Mode mode)
        : pattern_(std::move(pattern)), regex_(pattern_), mode_(mode)
    {}

    const std::string& getPattern() const override { return pattern_; }
    Mode getMode() const override { return mode_; }

    bool accepts(const LogRecord& record) const override
    {
        const bool hasMatch = std::regex_search(record.message, regex_);
        return mode_ == Mode::Exclude ? !hasMatch : hasMatch;
    }

    std::string pattern_;
    std::regex regex_;
    Mode mode_;
};

} // namespace

std::shared_ptr<RegexFilter> makeRegexFilter(std::string pattern, FilterMode mode)
{
    return std::make_shared<RegexFilterImpl>(std::move(pattern), mode);
}

}} // namespace sk::logger
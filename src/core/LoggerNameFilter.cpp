/**
 * @file LoggerNameFilter.cpp
 * @brief Internal logger-name filter implementation.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 18, 2026
 */
#include "BuiltinFilterFactory.h"

#include <LogRecord.h>
#include <algorithm>
#include <utility>

namespace sk { namespace logger {
namespace {

class LoggerNameFilterImpl final : public LoggerNameFilter {
public:
    LoggerNameFilterImpl(std::vector<std::string> names, Mode mode)
        : names_(std::move(names)), mode_(mode)
    {}

    const std::vector<std::string>& getNames() const override { return names_; }
    Mode getMode() const override { return mode_; }

    bool accepts(const LogRecord& record) const override
    {
        if (names_.empty())
            return true;

        const bool hasMatch = std::find(names_.begin(), names_.end(), record.loggerName)
                              != names_.end();
        return mode_ == Mode::Exclude ? !hasMatch : hasMatch;
    }

    std::vector<std::string> names_;
    Mode mode_;
};

} // namespace

std::shared_ptr<LoggerNameFilter> makeLoggerNameFilter(
    std::vector<std::string> names, FilterMode mode)
{
    return std::make_shared<LoggerNameFilterImpl>(std::move(names), mode);
}

}} // namespace sk::logger
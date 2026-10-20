/**
 * @file GlobalOstreamSink.cpp
 * @brief Thread-safe global ostream sink registration and dispatch.
 * 
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 18, 2026
 */
#include "GlobalOstreamSink.h"

#include "FilterEvaluator.h"
#include "LogRecordPattern.h"
#include "LoggerBase.h"
#include "LoggerUtils.h"

#include <mutex>
#include <stdexcept>
#include <utility>

namespace sk { namespace logger {
namespace {

class OstreamSink {
public:
    OstreamSink(std::ostream& stream, std::string pattern,
                                std::vector<std::shared_ptr<const Filter>> filters,
                                Logger::Level level)
                : stream_(&stream), pattern_(std::move(pattern)), filters_(std::move(filters)),
                    level_(level)
    {}

    void write(const LogRecord& record, std::optional<Logger::Level> flushOn)
    {
                if (record.level > level_)
                        return;
        if (!FilterEvaluator::passesAll(filters_, record))
            return;

        std::lock_guard<std::mutex> lock(mutex_);
        if (pattern_.empty()) {
            *stream_ << LoggerBase::levelToString(record.level)
                     << " [" << record.loggerName << "] "
                     << record.message << eol;
        } else {
            *stream_ << LogRecordPattern::render(pattern_, record);
        }
        if (flushOn.has_value() && record.level <= *flushOn)
            stream_->flush();
    }

private:
    std::ostream* stream_;
    std::string pattern_;
    std::vector<std::shared_ptr<const Filter>> filters_;
    Logger::Level level_;
    std::mutex mutex_;
};

std::mutex& registryMutex()
{
    static std::mutex mutex;
    return mutex;
}

std::vector<std::shared_ptr<OstreamSink>>& registry()
{
    static std::vector<std::shared_ptr<OstreamSink>> sinks;
    return sinks;
}

} // namespace

void addGlobalOstreamSink(std::ostream& stream, std::string pattern,
                          std::vector<std::shared_ptr<const Filter>> filters,
                          Logger::Level level)
{
    for (const auto& filter : filters) {
        if (!filter)
            throw std::invalid_argument("Global ostream sink cannot contain a null filter");
    }

    auto sink = std::make_shared<OstreamSink>(stream, std::move(pattern),
                                              std::move(filters), level);
    std::lock_guard<std::mutex> lock(registryMutex());
    registry().push_back(std::move(sink));
}

void clearGlobalOstreamSinks()
{
    std::lock_guard<std::mutex> lock(registryMutex());
    registry().clear();
}

void dispatchGlobalOstreamSinks(const LogRecord& record,
                               std::optional<Logger::Level> flushOn)
{
    std::vector<std::shared_ptr<OstreamSink>> sinks;
    {
        std::lock_guard<std::mutex> lock(registryMutex());
        sinks = registry();
    }
    for (const auto& sink : sinks)
        sink->write(record, flushOn);
}

}} // namespace sk::logger
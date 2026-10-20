/**
 * @file FilterSink.h
 * @brief spdlog sink wrapper that evaluates polymorphic filters.
 * 
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 18, 2026
 */
#ifndef SK_SPDLOG_FILTER_SINK_H
#define SK_SPDLOG_FILTER_SINK_H

#include <FilterEvaluator.h>
#include <LogRecord.h>
#include <SpdlogThreadLocal.h>
#include <logger/Filter.h>
#include <spdlog/sinks/sink.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sk { namespace logger { namespace spdlog_sinks {

class FilterSink : public spdlog::sinks::sink
{
public:
    explicit FilterSink(std::shared_ptr<spdlog::sinks::sink> inner,
                        std::vector<std::shared_ptr<const Filter>> filters)
        : inner_(std::move(inner)), filters_(std::move(filters))
    {
        set_level(inner_->level());
    }

    void log(const spdlog::details::log_msg& msg) override
    {
        LogRecord record{};
        record.level = Logger::Level::Info;
        record.loggerName.assign(msg.logger_name.data(), msg.logger_name.size());
        record.message.assign(msg.payload.data(), msg.payload.size());
        if (spdlog_tls::markers)
            record.markers = *spdlog_tls::markers;
        if (FilterEvaluator::passesAll(filters_, record))
            inner_->log(msg);
    }

    void flush() override { inner_->flush(); }
    void set_pattern(const std::string& pattern) override { inner_->set_pattern(pattern); }
    void set_formatter(std::unique_ptr<spdlog::formatter> formatter) override
    {
        inner_->set_formatter(std::move(formatter));
    }

private:
    std::shared_ptr<spdlog::sinks::sink> inner_;
    std::vector<std::shared_ptr<const Filter>> filters_;
};

}}} // namespace sk::logger::spdlog_sinks

#endif // SK_SPDLOG_FILTER_SINK_H
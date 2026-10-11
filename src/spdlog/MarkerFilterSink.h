/**
 * @file MarkerFilterSink.h
 * @brief spdlog sink wrapper that filters log records by marker.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 4, 2026
 */
#ifndef SK_SPDLOG_MARKER_FILTER_SINK_H
#define SK_SPDLOG_MARKER_FILTER_SINK_H

#include <spdlog/sinks/sink.h>
#include <MarkerFilterConfig.h>
#include <memory>

namespace sk { namespace logger { namespace spdlog_sinks {

/**
 * @class MarkerFilterSink
 * @brief Wrapper sink that gates spdlog dispatch based on marker membership.
 *
 * Delegates to an inner sink only when the current log record's markers
 * satisfy the filter criteria.  Uses thread-local marker vector set by
 * SpdlogLogger::append().
 */
class MarkerFilterSink : public spdlog::sinks::sink
{
public:
    explicit MarkerFilterSink(std::shared_ptr<spdlog::sinks::sink> inner,
                              MarkerFilterConfig filter)
        : m_inner(std::move(inner)), m_filter(std::move(filter))
    {
        set_level(m_inner->level());
    }

    void log(const spdlog::details::log_msg& msg) override
    {
        const auto& markers = spdlog_tls::markers
                                  ? *spdlog_tls::markers
                                  : std::vector<const Marker*>{};
        if (!m_filter.passesAny(markers))
            return;
        m_inner->log(msg);
    }

    void flush() override { m_inner->flush(); }

    // Forward pattern/formatter setters to inner sink
    void set_pattern(const std::string& pat) override { m_inner->set_pattern(pat); }
    void set_formatter(std::unique_ptr<spdlog::formatter> fmt) override
    {
        m_inner->set_formatter(std::move(fmt));
    }

private:
    std::shared_ptr<spdlog::sinks::sink> m_inner;
    MarkerFilterConfig m_filter;
};

}}} // namespace sk::logger::spdlog_sinks

#endif // SK_SPDLOG_MARKER_FILTER_SINK_H
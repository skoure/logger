/**
 * @file Log4CxxMarkerFilter.h
 * @brief log4cxx spi::Filter implementation for marker-based filtering.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 4, 2026
 */
#ifndef SK_LOG4CXX_MARKER_FILTER_H
#define SK_LOG4CXX_MARKER_FILTER_H

#include <MarkerFilterConfig.h>
#include <log4cxx/spi/filter.h>
#include <log4cxx/logstring.h>
#include <log4cxx/helpers/transcoder.h>
#include <log4cxx/helpers/pool.h>
#include <sstream>
#include <string>
#include <vector>

namespace sk { namespace logger {

/**
 * @class Log4CxxMarkerFilter
 * @brief log4cxx Filter that accepts/rejects events based on marker names.
 *
 * Reads the comma-joined marker names from the thread MDC key "marker"
 * (set by Log4CxxLogger::append before the LOG4CXX_* macro call).
 * Mirrors MarkerFilter semantics: include (default) — event must carry at
 * least one listed marker; exclude — events carrying any listed marker are
 * suppressed; unmarked events always pass in exclude mode.
 */
class Log4CxxMarkerFilter : public log4cxx::spi::Filter
{
public:
    explicit Log4CxxMarkerFilter(const MarkerFilterConfig& filter)
        : m_filter(filter) {}

    FilterDecision decide(const log4cxx::spi::LoggingEventPtr& event) const override
    {
        if (m_filter.empty())
            return NEUTRAL;

        log4cxx::LogString mdcValue;
        event->getMDC(log4cxx::LogString(LOG4CXX_STR("marker")), mdcValue);

        if (mdcValue.empty())
            return m_filter.exclude ? NEUTRAL : DENY;

        std::string names;
        LOG4CXX_NS::helpers::Transcoder::encodeUTF8(mdcValue, names);
        const std::vector<std::string> eventMarkers = splitMarkerNames(names);

        if (m_filter.exclude)
        {
            for (const auto& name : eventMarkers)
            {
                if (m_filter.matches(name))
                    return DENY;
            }
            return NEUTRAL;
        }

        // include mode: event passes only if at least one marker matches
        for (const auto& name : eventMarkers)
        {
            if (m_filter.matches(name))
                return NEUTRAL;
        }
        return DENY;
    }

private:
    MarkerFilterConfig m_filter;

    static std::vector<std::string> splitMarkerNames(const std::string& joined)
    {
        std::vector<std::string> result;
        std::stringstream ss(joined);
        std::string item;
        while (std::getline(ss, item, ','))
        {
            // trim leading/trailing whitespace
            std::size_t start = item.find_first_not_of(" \t");
            if (start == std::string::npos)
                continue;
            std::size_t end = item.find_last_not_of(" \t");
            result.push_back(item.substr(start, end - start + 1));
        }
        return result;
    }
};

}} // namespace sk::logger

#endif // SK_LOG4CXX_MARKER_FILTER_H
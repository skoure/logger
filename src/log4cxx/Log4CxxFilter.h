/**
 * @file Log4CxxFilter.h
 * @brief log4cxx appender filter that evaluates polymorphic filters.
 * 
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 18, 2026
 */
#ifndef SK_LOG4CXX_FILTER_H
#define SK_LOG4CXX_FILTER_H

#include <FilterEvaluator.h>
#include <LogRecord.h>
#include <logger/Filter.h>
#include <logger/MarkerFactory.h>
#include <log4cxx/spi/filter.h>
#include <log4cxx/logstring.h>
#include <log4cxx/helpers/transcoder.h>

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace sk { namespace logger {

class Log4CxxFilter : public log4cxx::spi::Filter
{
public:
    explicit Log4CxxFilter(
        std::vector<std::shared_ptr<const sk::logger::Filter>> filters)
        : filters_(std::move(filters))
    {}

    FilterDecision decide(const log4cxx::spi::LoggingEventPtr& event) const override
    {
        if (filters_.empty())
            return NEUTRAL;

        LogRecord record{};
        record.level = Logger::Level::Info;
        LOG4CXX_NS::helpers::Transcoder::encodeUTF8(
            event->getLoggerName(), record.loggerName);
        LOG4CXX_NS::helpers::Transcoder::encodeUTF8(
            event->getMessage(), record.message);

        log4cxx::LogString mdcValue;
        event->getMDC(log4cxx::LogString(LOG4CXX_STR("marker")), mdcValue);
        std::string markerNames;
        LOG4CXX_NS::helpers::Transcoder::encodeUTF8(mdcValue, markerNames);

        std::vector<std::shared_ptr<const Marker>> markerHolds;
        std::stringstream stream(markerNames);
        std::string name;
        while (std::getline(stream, name, ',')) {
            const std::size_t first = name.find_first_not_of(" \t");
            if (first == std::string::npos)
                continue;
            const std::size_t last = name.find_last_not_of(" \t");
            name = name.substr(first, last - first + 1);
            markerHolds.push_back(MarkerFactory::getMarker(name));
        }
        for (const auto& marker : markerHolds)
            record.markers.push_back(marker.get());

        return FilterEvaluator::passesAll(filters_, record) ? NEUTRAL : DENY;
    }

private:
    std::vector<std::shared_ptr<const sk::logger::Filter>> filters_;
};

}} // namespace sk::logger

#endif // SK_LOG4CXX_FILTER_H
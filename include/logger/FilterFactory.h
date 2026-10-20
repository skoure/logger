/**
 * @file FilterFactory.h
 * @brief Factory for constructing built-in log filters.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 18, 2026
 */
#ifndef SK_LOGGER_FILTER_FACTORY_H
#define SK_LOGGER_FILTER_FACTORY_H

#include <logger/LoggerNameFilter.h>
#include <logger/MarkerFilter.h>
#include <logger/RegexFilter.h>
#include <memory>
#include <string>
#include <vector>

namespace sk { namespace logger {

/**
 * @class FilterFactory
 * @brief Constructs the built-in sink filter types.
 */
class FilterFactory {
public:
    /**
     * @brief Create a marker filter.
     *
     * An event matches when any attached marker name equals any configured
     * marker name. An empty marker list imposes no filtering.
     *
     * @param markers Marker instances to match.
     * @param mode    Include matching events or exclude them; defaults to include.
     * @return A mutable marker filter that can be passed as a `Filter`.
     * @throws std::invalid_argument If a marker is null or @p mode is invalid.
     */
    static std::shared_ptr<MarkerFilter> createMarkerFilter(
        std::vector<std::shared_ptr<const Marker>> markers = {},
        FilterMode mode = FilterMode::Include);

    /**
     * @brief Create a regular-expression filter for the raw message body.
     *
     * Uses ECMAScript syntax and substring search (`std::regex_search`). Use
     * anchors such as `^` and `$` when matching must be position-constrained.
     *
     * @param pattern ECMAScript regular-expression pattern.
     * @param mode    Include matching messages or exclude them; defaults to include.
     * @return A regex filter that can be passed as a `Filter`.
     * @throws std::regex_error If @p pattern is not a valid regular expression.
     * @throws std::invalid_argument If @p mode is invalid.
     */
    static std::shared_ptr<RegexFilter> createRegexFilter(
        std::string pattern,
        FilterMode mode = FilterMode::Include);

    /**
     * @brief Create a filter for exact logger-name matches.
     *
     * Matching is case-sensitive; a name does not implicitly match descendants.
     * An empty name list imposes no filtering.
     *
     * @param names Full logger names to match.
     * @param mode  Include matching names or exclude them; defaults to include.
     * @return A logger-name filter that can be passed as a `Filter`.
     * @throws std::invalid_argument If @p mode is invalid.
     */
    static std::shared_ptr<LoggerNameFilter> createLoggerNameFilter(
        std::vector<std::string> names,
        FilterMode mode = FilterMode::Include);

private:
    FilterFactory() = delete;
    ~FilterFactory() = delete;
};

}} // namespace sk::logger

#endif // SK_LOGGER_FILTER_FACTORY_H
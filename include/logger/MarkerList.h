/**
 * @file MarkerList.h
 * @brief Public interface for passing multiple markers to a single log call.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: September 20, 2026
 */
#ifndef SK_LOGGER_MARKERLIST_H
#define SK_LOGGER_MARKERLIST_H

#include <logger/Marker.h>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <vector>

namespace sk { namespace logger {

/**
 * @class MarkerList
 * @brief Lightweight, non-owning list of markers for multi-marker log calls.
 *
 * Enables call sites like:
 * @code
 *   auto sql   = sk::logger::MarkerFactory::getMarker("SQL");
 *   auto audit = sk::logger::MarkerFactory::getMarker("AUDIT");
 *   log->info({*sql, *audit}, "query took %d ms", ms);
 * @endcode
 *
 * A braced list of marker references or pointers implicitly converts to
 * MarkerList. An empty braced list produces an empty MarkerList (equivalent
 * to logging without a marker). Marker objects are owned by the MarkerFactory
 * registry; MarkerList stores non-owning pointers, so all listed markers must
 * outlive the log call — which is guaranteed for factory-created markers.
 */
class MarkerList {
public:
    /** @brief Empty list. */
    MarkerList() = default;

    /** @brief Construct from a braced list of marker references, e.g. {@code {*sql, *audit}}. */
    MarkerList(std::initializer_list<std::reference_wrapper<const Marker>> markers)
    {
        m_markers.reserve(markers.size());
        for (const Marker& marker : markers)
            m_markers.push_back(&marker);
    }

    /** @brief Construct from a braced list of marker pointers, e.g. {@code {sql, audit}}. */
    MarkerList(std::initializer_list<const Marker*> markers)
        : m_markers(markers)
    {
    }

    /** @brief Construct from a vector of marker pointers, e.g. {@code {p1, p2}}. */
    MarkerList(const std::vector<const Marker*>& markers)
        : m_markers(markers)
    {
    }

    /** @brief Returns the underlying markers in the order they were listed. */
    const std::vector<const Marker*>& get() const { return m_markers; }

    /** @brief Returns true if no markers were listed. */
    bool empty() const { return m_markers.empty(); }

    /** @brief Returns the number of markers in the list. */
    std::size_t size() const { return m_markers.size(); }

private:
    std::vector<const Marker*> m_markers;
};

}} // namespace sk::logger

#endif // SK_LOGGER_MARKERLIST_H

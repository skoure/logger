/**
 * @file Filter.h
 * @brief Common polymorphic base for sink filters.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 18, 2026
 */
#ifndef SK_LOGGER_FILTER_H
#define SK_LOGGER_FILTER_H

namespace sk { namespace logger {

struct LogRecord;
class FilterEvaluator;

/** Include matching events or exclude matching events. */
enum class FilterMode {
    /** Keep events that match the filter. */
    Include,
    /** Drop events that match the filter. */
    Exclude
};

/**
 * @class Filter
 * @brief Common polymorphic base for factory-created sink filters.
 *
 * Callers configure a concrete filter and pass it through APIs accepting
 * this base type. Record evaluation is private to the logger library.
 */
class Filter {
public:
    using Mode = FilterMode;

    /** Virtual destructor for polymorphic filter objects. */
    virtual ~Filter() = default;

    virtual bool accepts(const LogRecord& record) const = 0;
};

}} // namespace sk::logger

#endif // SK_LOGGER_FILTER_H
/**
 * @file LoggerNameFilter.h
 * @brief Public configuration interface for logger-name filters.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 18, 2026
 */
#ifndef SK_LOGGER_NAME_FILTER_H
#define SK_LOGGER_NAME_FILTER_H

#include <logger/Filter.h>
#include <string>
#include <vector>

namespace sk { namespace logger {

class LoggerNameFilter : public Filter {
public:
    using Mode = FilterMode;

    /**
     * @return The exact logger names tested by this filter. A matching event
     *         is one whose full logger name equals any entry in this list.
     */
    virtual const std::vector<std::string>& getNames() const = 0;

    /** @return Whether matching logger names are included or excluded. */
    virtual Mode getMode() const = 0;
};

}} // namespace sk::logger

#endif // SK_LOGGER_NAME_FILTER_H
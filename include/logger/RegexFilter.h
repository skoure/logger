/**
 * @file RegexFilter.h
 * @brief Public configuration interface for message regular-expression filters.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 18, 2026
 */
#ifndef SK_LOGGER_REGEX_FILTER_H
#define SK_LOGGER_REGEX_FILTER_H

#include <logger/Filter.h>
#include <string>

namespace sk { namespace logger {

class RegexFilter : public Filter {
public:
    using Mode = FilterMode;

    /** @return The ECMAScript regular expression used to search the message body. */
    virtual const std::string& getPattern() const = 0;

    /** @return Whether matching messages are included or excluded. */
    virtual Mode getMode() const = 0;
};

}} // namespace sk::logger

#endif // SK_LOGGER_REGEX_FILTER_H
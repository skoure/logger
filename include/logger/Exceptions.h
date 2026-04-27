/**
 * @file Exceptions.h
 * @brief Logger exception types.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: April 26, 2026
 */
#ifndef SK_LOGGER_EXCEPTIONS_H
#define SK_LOGGER_EXCEPTIONS_H

#include <stdexcept>

namespace sk { namespace logger {

/**
 * @class ParseException
 * @brief Thrown for JSON configuration parsing or I/O errors.
 *
 * Extends std::runtime_error so callers can catch it specifically or fall back
 * to catching std::runtime_error for generic error handling.
 *
 * Thrown by:
 * - LoggerFactory::configureFromJsonFile()
 * - LoggerFactory::configureFromJsonString()
 */
class ParseException : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

}} // namespace sk::logger

#endif // SK_LOGGER_EXCEPTIONS_H

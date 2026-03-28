/**
 * @file Log4CxxPatternTranslator.h
 * @brief Translates canonical log4j-style patterns to log4cxx format strings.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 *
 * Most canonical tokens are already native log4cxx and pass through unchanged.
 * The following require transformation:
 *
 *   %d{strftime-fmt} → %d{java-date-fmt}
 *     e.g. %d{%Y-%m-%d %H:%M:%S} → %d{yyyy-MM-dd HH:mm:ss}
 *
 *   %M  → %X{marker}    (reads MDC "marker" key set by Log4CxxLogger::append())
 *   %T  → %t            (log4cxx %t = thread name — maps our thread-name token)
 *
 * strftime→java mapping for date tokens:
 *   %Y → yyyy   %m → MM   %d → dd
 *   %H → HH     %M → mm   %S → ss
 *
 * Note: %M appears in two contexts.
 *   • As a top-level token     → %X{marker}
 *   • Inside %d{…}             → translated to java 'mm' (minutes)
 *   These are distinguished by context in the parser.
 */
#ifndef SK_LOG4CXX_PATTERN_TRANSLATOR_H
#define SK_LOG4CXX_PATTERN_TRANSLATOR_H

#include <string>

namespace sk { namespace logger {

/**
 * @class Log4CxxPatternTranslator
 * @brief Stateless translator from canonical log4j pattern to log4cxx pattern.
 */
class Log4CxxPatternTranslator
{
public:
    /**
     * @brief Translate a canonical log4j-style pattern to a log4cxx pattern.
     * @param canonical Pattern using canonical tokens (see file header).
     * @return log4cxx-compatible pattern string.
     */
    static std::string translate(const std::string& canonical);

private:
    Log4CxxPatternTranslator() = delete;
    ~Log4CxxPatternTranslator() = delete;
};

}} // namespace sk::logger

#endif // SK_LOG4CXX_PATTERN_TRANSLATOR_H

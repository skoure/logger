/**
 * @file Log4CxxPatternTranslator.h
 * @brief Translates canonical log4j-style patterns to log4cxx format strings.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 *
 * Most canonical tokens map to native log4cxx tokens. Marker tokens are
 * represented by a per-token converter so each occurrence can use its own
 * parsed selection and separator options.
 *
 *   %d{strftime-fmt} → %d{java-date-fmt}
 *     e.g. %d{%Y-%m-%d %H:%M:%S} → %d{yyyy-MM-dd HH:mm:ss}
 *
 *   %T  → %t            (log4cxx %t = thread name — maps our thread-name token)
 *
 * strftime→java mapping for date tokens:
 *   %Y → yyyy   %m → MM   %d → dd
 *   %H → HH     %M → mm   %S → ss
 *
 * Note: %M inside %d{...} is a strftime minute specifier and becomes java 'mm';
 * only top-level %M tokens are treated as library marker tokens.
 */
#ifndef SK_LOG4CXX_PATTERN_TRANSLATOR_H
#define SK_LOG4CXX_PATTERN_TRANSLATOR_H

#include "MarkerPattern.h"

#include <string>
#include <vector>

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

    /**
     * @brief Translates a pattern and returns its configured marker rules.
     * @param canonical Pattern using canonical tokens.
     * @param markerOptions Receives one rule per configured `%M{...}` token.
     * @return log4cxx-compatible pattern string.
     */
    static std::string translate(const std::string& canonical,
                                 std::vector<MarkerPatternOptions>& markerOptions);

private:
    Log4CxxPatternTranslator() = delete;
    ~Log4CxxPatternTranslator() = delete;
};

}} // namespace sk::logger

#endif // SK_LOG4CXX_PATTERN_TRANSLATOR_H

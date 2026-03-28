/**
 * @file SpdlogPatternTranslator.h
 * @brief Translates canonical log4j-style patterns to spdlog format strings.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 *
 * Canonical token → spdlog equivalent:
 *   %m  → %v   (message)
 *   %p  → %l   (level)
 *   %c  → %n   (logger name)
 *   %t  → %t   (thread id, native in spdlog)
 *   %T  → %*   (thread name — custom flag, see SpdlogBackend.cpp)
 *   %M  → %&   (marker    — custom flag, see SpdlogBackend.cpp)
 *   %d{strftime-fmt} → strftime-fmt inline (spdlog parses strftime directly)
 *   %n  → %n   (newline — no, spdlog %n is logger name; use \n literal)
 *   other tokens pass through unchanged
 */
#ifndef SK_SPDLOG_PATTERN_TRANSLATOR_H
#define SK_SPDLOG_PATTERN_TRANSLATOR_H

#include <string>

namespace sk { namespace logger {

/**
 * @class SpdlogPatternTranslator
 * @brief Stateless translator from canonical log4j pattern to spdlog pattern.
 */
class SpdlogPatternTranslator
{
public:
    /**
     * @brief Translate a canonical log4j-style pattern to a spdlog pattern.
     * @param canonical Pattern using canonical tokens (see file header).
     * @return spdlog-compatible pattern string.
     */
    static std::string translate(const std::string& canonical);

private:
    SpdlogPatternTranslator() = delete;
    ~SpdlogPatternTranslator() = delete;
};

}} // namespace sk::logger

#endif // SK_SPDLOG_PATTERN_TRANSLATOR_H

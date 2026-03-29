/**
 * @file SpdlogPatternTranslator.cpp
 * @brief Translates canonical log4j-style patterns to spdlog format strings.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 */
#include "SpdlogPatternTranslator.h"
#include <stdexcept>

using namespace sk::logger;

// ---------------------------------------------------------------------------
// translate()
// ---------------------------------------------------------------------------

std::string SpdlogPatternTranslator::translate(const std::string& canonical)
{
    std::string result;
    result.reserve(canonical.size() * 2);

    std::size_t i = 0;
    const std::size_t n = canonical.size();

    while (i < n)
    {
        if (canonical[i] != '%')
        {
            result += canonical[i++];
            continue;
        }

        // We have a '%'
        if (i + 1 >= n)
        {
            result += '%';
            ++i;
            continue;
        }

        char next = canonical[i + 1];

        switch (next)
        {
        case 'm':
            result += "%v";
            i += 2;
            break;

        case 'p':
            result += "%l";
            i += 2;
            break;

        case 'c':
            result += "%n";
            i += 2;
            break;

        case 't':
            // thread id — native in spdlog
            result += "%t";
            i += 2;
            break;

        case 'T':
            // thread name — custom flag character '*'
            result += "%*";
            i += 2;
            break;

        case 'M':
            // marker — custom flag character '&'
            result += "%&";
            i += 2;
            break;

        case 'n':
            // canonical %n = newline; spdlog %n = logger name, so emit literal \n
            result += "\n";
            i += 2;
            break;

        case 'd':
        {
            // %d{strftime-fmt} → strip the wrapper and emit strftime tokens inline
            // spdlog's pattern formatter accepts strftime tokens directly between
            // its other directives.
            i += 2; // skip '%d'
            if (i < n && canonical[i] == '{')
            {
                std::size_t close = canonical.find('}', i + 1);
                if (close == std::string::npos)
                    throw std::runtime_error(
                        "SpdlogPatternTranslator: unclosed '{' in %d{...}");
                // Emit the inner format string verbatim
                result += canonical.substr(i + 1, close - i - 1);
                i = close + 1;
            }
            else
            {
                // bare %d — emit spdlog's default date/time token
                result += "%Y-%m-%d %H:%M:%S";
            }
            break;
        }

        default:
            // Unknown token — pass through unchanged
            result += '%';
            result += next;
            i += 2;
            break;
        }
    }

    return result;
}

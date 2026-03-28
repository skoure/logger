/**
 * @file Log4CxxPatternTranslator.cpp
 * @brief Translates canonical log4j-style patterns to log4cxx format strings.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 */
#include "Log4CxxPatternTranslator.h"
#include <stdexcept>

using namespace sk::logger;

// ---------------------------------------------------------------------------
// Helper: translate strftime-style date format to java date format
// ---------------------------------------------------------------------------

static std::string strftimeToJava(const std::string& strftimeFmt)
{
    std::string result;
    result.reserve(strftimeFmt.size() * 2);

    const std::size_t n = strftimeFmt.size();
    std::size_t i = 0;

    while (i < n)
    {
        if (strftimeFmt[i] != '%')
        {
            result += strftimeFmt[i++];
            continue;
        }

        if (i + 1 >= n)
        {
            result += '%';
            ++i;
            continue;
        }

        char next = strftimeFmt[i + 1];
        i += 2;

        switch (next)
        {
        case 'Y': result += "yyyy"; break;
        case 'm': result += "MM";   break;  // month (in strftime context)
        case 'd': result += "dd";   break;
        case 'H': result += "HH";   break;
        case 'M': result += "mm";   break;  // minutes (in strftime context)
        case 'S': result += "ss";   break;
        default:
            result += '%';
            result += next;
            break;
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// Log4CxxPatternTranslator::translate
// ---------------------------------------------------------------------------

std::string Log4CxxPatternTranslator::translate(const std::string& canonical)
{
    std::string result;
    result.reserve(canonical.size() * 2);

    const std::size_t n = canonical.size();
    std::size_t i = 0;

    while (i < n)
    {
        if (canonical[i] != '%')
        {
            result += canonical[i++];
            continue;
        }

        if (i + 1 >= n)
        {
            result += '%';
            ++i;
            continue;
        }

        char next = canonical[i + 1];

        switch (next)
        {
        // Native log4cxx tokens — pass through unchanged
        case 'm': result += "%m"; i += 2; break;
        case 'p': result += "%p"; i += 2; break;
        case 'c': result += "%c"; i += 2; break;
        case 't': result += "%t"; i += 2; break;  // canonical %t = thread id; log4cxx %t = thread name (close enough)
        case 'n': result += "%n"; i += 2; break;

        case 'T':
            // Canonical %T = thread name; log4cxx %t = thread name
            result += "%t";
            i += 2;
            break;

        case 'M':
            // Canonical %M = marker name; read from MDC "marker" key
            result += "%X{marker}";
            i += 2;
            break;

        case 'd':
        {
            i += 2; // skip '%d'
            if (i < n && canonical[i] == '{')
            {
                std::size_t close = canonical.find('}', i + 1);
                if (close == std::string::npos)
                    throw std::runtime_error(
                        "Log4CxxPatternTranslator: unclosed '{' in %d{...}");

                std::string strftimeFmt = canonical.substr(i + 1, close - i - 1);
                i = close + 1;

                result += "%d{";
                result += strftimeToJava(strftimeFmt);
                result += '}';
            }
            else
            {
                // Bare %d — use log4cxx default
                result += "%d";
            }
            break;
        }

        default:
            // Unknown token — pass through
            result += '%';
            result += next;
            i += 2;
            break;
        }
    }

    return result;
}

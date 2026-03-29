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
// Internal modifier support
// ---------------------------------------------------------------------------

namespace {

struct PatternModifier {
    bool leftAlign = false;
    int  minWidth  = 0;   // 0 = no minimum
    int  maxWidth  = -1;  // -1 = no maximum
};

static PatternModifier parseModifier(const std::string& s, std::size_t& i, std::size_t n)
{
    PatternModifier mod;
    if (i < n && s[i] == '-') { mod.leftAlign = true; ++i; }
    while (i < n && std::isdigit(static_cast<unsigned char>(s[i])))
        mod.minWidth = mod.minWidth * 10 + (s[i++] - '0');
    if (i < n && s[i] == '.') {
        ++i;
        mod.maxWidth = 0;
        while (i < n && std::isdigit(static_cast<unsigned char>(s[i])))
            mod.maxWidth = mod.maxWidth * 10 + (s[i++] - '0');
    }
    return mod;
}

static std::string modifierString(const PatternModifier& mod)
{
    std::string s;
    if (mod.leftAlign)    s += '-';
    if (mod.minWidth > 0) s += std::to_string(mod.minWidth);
    if (mod.maxWidth >= 0) { s += '.'; s += std::to_string(mod.maxWidth); }
    return s;
}

} // namespace

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

        // Trailing '%' with nothing after it
        if (i + 1 >= n)
        {
            result += '%';
            ++i;
            continue;
        }

        ++i; // consume '%'

        PatternModifier mod = parseModifier(canonical, i, n);

        if (i >= n)
        {
            // '%' followed only by modifier chars at end of string — pass through raw
            result += '%';
            result += modifierString(mod);
            break;
        }

        char token = canonical[i++];
        std::string modStr = modifierString(mod);

        switch (token)
        {
        // Native log4cxx tokens — modifier passes through verbatim (same syntax)
        case 'm': result += '%' + modStr + 'm'; break;
        case 'p': result += '%' + modStr + 'p'; break;
        case 'c': result += '%' + modStr + 'c'; break;
        case 't': result += '%' + modStr + 't'; break;  // canonical %t = thread id; log4cxx %t = thread name (close enough)
        case 'n': result += '%' + modStr + 'n'; break;

        case 'T':
            // Canonical %T = thread name; log4cxx %t = thread name
            result += '%' + modStr + 't';
            break;

        case 'M':
            // Canonical %M = marker name; read from MDC "marker" key
            // log4cxx handles %-10X{marker} natively
            result += '%' + modStr + "X{marker}";
            break;

        case 'd':
        {
            if (i < n && canonical[i] == '{')
            {
                std::size_t close = canonical.find('}', i + 1);
                if (close == std::string::npos)
                    throw std::runtime_error(
                        "Log4CxxPatternTranslator: unclosed '{' in %d{...}");

                std::string strftimeFmt = canonical.substr(i + 1, close - i - 1);
                i = close + 1;

                // Modifier prefix goes before d{...}: %-25d{yyyy-MM-dd HH:mm:ss}
                result += '%' + modStr + "d{";
                result += strftimeToJava(strftimeFmt);
                result += '}';
            }
            else
            {
                // Bare %d — use log4cxx default, modifier passes through
                result += '%' + modStr + 'd';
            }
            break;
        }

        default:
            // Unknown token — pass through with modifier unchanged
            result += '%' + modStr + token;
            break;
        }
    }

    return result;
}

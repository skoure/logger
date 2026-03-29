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
    if (mod.leftAlign)   s += '-';
    if (mod.minWidth > 0) s += std::to_string(mod.minWidth);
    if (mod.maxWidth >= 0) { s += '.'; s += std::to_string(mod.maxWidth); }
    return s;
}

} // namespace

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
        case 'm':
            result += '%' + modStr + 'v';
            break;

        case 'p':
            result += '%' + modStr + 'l';
            break;

        case 'c':
            result += '%' + modStr + 'n';
            break;

        case 't':
            // thread id — native in spdlog
            result += '%' + modStr + 't';
            break;

        case 'T':
            // thread name — custom flag character '*'
            result += '%' + modStr + '*';
            break;

        case 'M':
            // marker — custom flag character '&'
            result += '%' + modStr + '&';
            break;

        case 'n':
            // canonical %n = newline; spdlog %n = logger name, so emit literal \n
            // Modifier is ignored — newline is always a single character
            result += "\n";
            break;

        case 'd':
        {
            // %d{strftime-fmt} → strip the wrapper and emit strftime tokens inline
            // spdlog's pattern formatter accepts strftime tokens directly between
            // its other directives.
            // Note: a width modifier on %d cannot be propagated to a multi-token
            // strftime sequence in spdlog's format — the modifier is silently dropped.
            if (i < n && canonical[i] == '{')
            {
                std::size_t close = canonical.find('}', i + 1);
                if (close == std::string::npos)
                    throw std::runtime_error(
                        "SpdlogPatternTranslator: unclosed '{' in %d{...}");
                // Emit the inner format string verbatim (modifier dropped)
                result += canonical.substr(i + 1, close - i - 1);
                i = close + 1;
            }
            else
            {
                // bare %d — emit spdlog's default date/time token (modifier dropped)
                result += "%Y-%m-%d %H:%M:%S";
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

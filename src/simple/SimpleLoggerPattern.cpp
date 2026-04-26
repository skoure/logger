/**
 * @file SimpleLoggerPattern.cpp
 * @brief Pattern renderer for the SimpleLogger backend.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 */
#include "SimpleLoggerPattern.h"
#include "LoggerBase.h"
#include "LoggerUtils.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

using namespace sk::logger;

// ---------------------------------------------------------------------------
// Helper: format thread id as decimal string
// ---------------------------------------------------------------------------

static std::string threadIdToString(std::thread::id id)
{
    std::ostringstream oss;
    oss << id;
    return oss.str();
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

static std::string applyModifier(std::string value, const PatternModifier& mod)
{
    // Truncate from the left (log4j default: removes leading characters)
    if (mod.maxWidth >= 0 && static_cast<int>(value.size()) > mod.maxWidth)
        value = value.substr(value.size() - static_cast<std::size_t>(mod.maxWidth));

    // Pad with spaces to reach minimum width
    if (mod.minWidth > 0 && static_cast<int>(value.size()) < mod.minWidth) {
        int pad = mod.minWidth - static_cast<int>(value.size());
        if (mod.leftAlign) value.append(static_cast<std::size_t>(pad), ' ');
        else               value.insert(0, static_cast<std::size_t>(pad), ' ');
    }

    return value;
}

static std::string expandDate(const std::string& pattern,
                               std::size_t& i, std::size_t n,
                               const LogRecord& record)
{
    const char* fmt = "%Y-%m-%d %H:%M:%S";
    std::string fmtBuf;

    if (i < n && pattern[i] == '{')
    {
        std::size_t close = pattern.find('}', i + 1);
        if (close == std::string::npos)
        {
            // Malformed — emit literally and stop
            return "%d{";
        }
        fmtBuf = pattern.substr(i + 1, close - i - 1);
        i = close + 1;
        fmt = fmtBuf.c_str();
    }

    auto tt = std::chrono::system_clock::to_time_t(record.timestamp);
    std::tm tm_val;
#ifdef _WIN32
    localtime_s(&tm_val, &tt);
#else
    localtime_r(&tt, &tm_val);
#endif
    char buf[256];
    if (std::strftime(buf, sizeof(buf), fmt, &tm_val) > 0)
        return buf;
    return {};
}

} // namespace

// ---------------------------------------------------------------------------
// SimpleLoggerPattern::render
// ---------------------------------------------------------------------------

std::string SimpleLoggerPattern::render(const std::string& pattern,
                                        const LogRecord& record)
{
    std::string result;
    result.reserve(pattern.size() * 2);

    const std::size_t n = pattern.size();
    std::size_t i = 0;

    while (i < n)
    {
        if (pattern[i] != '%')
        {
            result += pattern[i++];
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

        PatternModifier mod = parseModifier(pattern, i, n);

        if (i >= n)
        {
            result += '%';
            break;
        }

        char token = pattern[i++];

        // %% — literal percent; modifier is ignored
        if (token == '%')
        {
            result += '%';
            continue;
        }

        switch (token)
        {
        case 'm':
            result += applyModifier(record.message, mod);
            break;

        case 'p':
            result += applyModifier(LoggerBase::levelToString(record.level), mod);
            break;

        case 'c':
            result += applyModifier(record.loggerName, mod);
            break;

        case 't':
            result += applyModifier(threadIdToString(record.threadId), mod);
            break;

        case 'T':
            result += applyModifier(record.threadName, mod);
            break;

        case 'M':
            result += applyModifier(
                record.marker ? record.marker->getName() : std::string{}, mod);
            break;

        case 'n':
            // Newline — modifier is ignored
            result += eol;
            break;

        case 'd':
            result += applyModifier(expandDate(pattern, i, n, record), mod);
            break;

        default:
            // Unknown token — pass through
            result += '%';
            result += token;
            break;
        }
    }

    return result;
}

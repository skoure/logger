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

        // We have '%'
        if (i + 1 >= n)
        {
            result += '%';
            ++i;
            continue;
        }

        char next = pattern[i + 1];

        switch (next)
        {
        case '%':
            result += '%';
            i += 2;
            break;

        case 'm':
            result += record.message;
            i += 2;
            break;

        case 'p':
            result += LoggerBase::levelToString(record.level);
            i += 2;
            break;

        case 'c':
            result += record.loggerName;
            i += 2;
            break;

        case 't':
            result += threadIdToString(record.threadId);
            i += 2;
            break;

        case 'T':
            result += record.threadName;
            i += 2;
            break;

        case 'M':
            if (record.marker)
                result += record.marker->getName();
            i += 2;
            break;

        case 'n':
            result += '\n';
            i += 2;
            break;

        case 'd':
        {
            i += 2; // skip '%d'
            if (i < n && pattern[i] == '{')
            {
                std::size_t close = pattern.find('}', i + 1);
                if (close == std::string::npos)
                {
                    // Malformed — emit literally
                    result += "%d{";
                    ++i;
                    break;
                }

                std::string fmt = pattern.substr(i + 1, close - i - 1);
                i = close + 1;

                // Convert timestamp to time_t then format with strftime
                auto tt = std::chrono::system_clock::to_time_t(record.timestamp);
                std::tm tm_val;
#ifdef _WIN32
                localtime_s(&tm_val, &tt);
#else
                localtime_r(&tt, &tm_val);
#endif
                char buf[256];
                if (std::strftime(buf, sizeof(buf), fmt.c_str(), &tm_val) > 0)
                    result += buf;
            }
            else
            {
                // bare %d — emit ISO timestamp
                auto tt = std::chrono::system_clock::to_time_t(record.timestamp);
                std::tm tm_val;
#ifdef _WIN32
                localtime_s(&tm_val, &tt);
#else
                localtime_r(&tt, &tm_val);
#endif
                char buf[64];
                std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_val);
                result += buf;
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

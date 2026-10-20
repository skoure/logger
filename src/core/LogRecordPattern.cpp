/**
 * @file LogRecordPattern.cpp
 * @brief Canonical pattern renderer shared by logger output paths.
 * 
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 18, 2026
 * 
 */
#include "LogRecordPattern.h"

#include "LoggerBase.h"
#include "LoggerUtils.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

using namespace sk::logger;

namespace {

struct PatternModifier {
    bool leftAlign = false;
    int minWidth = 0;
    int maxWidth = -1;
};

PatternModifier parseModifier(const std::string& pattern, std::size_t& index)
{
    PatternModifier modifier;
    if (index < pattern.size() && pattern[index] == '-') {
        modifier.leftAlign = true;
        ++index;
    }
    while (index < pattern.size()
           && std::isdigit(static_cast<unsigned char>(pattern[index])))
        modifier.minWidth = modifier.minWidth * 10 + (pattern[index++] - '0');
    if (index < pattern.size() && pattern[index] == '.') {
        ++index;
        modifier.maxWidth = 0;
        while (index < pattern.size()
               && std::isdigit(static_cast<unsigned char>(pattern[index])))
            modifier.maxWidth = modifier.maxWidth * 10 + (pattern[index++] - '0');
    }
    return modifier;
}

std::string applyModifier(std::string value, const PatternModifier& modifier)
{
    if (modifier.maxWidth >= 0
        && static_cast<int>(value.size()) > modifier.maxWidth)
        value = value.substr(value.size() - static_cast<std::size_t>(modifier.maxWidth));

    if (modifier.minWidth > 0
        && static_cast<int>(value.size()) < modifier.minWidth) {
        const std::size_t padding = static_cast<std::size_t>(
            modifier.minWidth - static_cast<int>(value.size()));
        if (modifier.leftAlign)
            value.append(padding, ' ');
        else
            value.insert(0, padding, ' ');
    }
    return value;
}

std::string threadIdToString(std::thread::id id)
{
    std::ostringstream stream;
    stream << id;
    return stream.str();
}

std::string expandDate(const std::string& pattern, std::size_t& index,
                       const LogRecord& record)
{
    std::string format = "%Y-%m-%d %H:%M:%S";
    if (index < pattern.size() && pattern[index] == '{') {
        const std::size_t close = pattern.find('}', index + 1);
        if (close == std::string::npos)
            return "%d{";
        format = pattern.substr(index + 1, close - index - 1);
        index = close + 1;
    }

    const auto time = std::chrono::system_clock::to_time_t(record.timestamp);
    std::tm localTime;
#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif
    char buffer[256];
    return std::strftime(buffer, sizeof(buffer), format.c_str(), &localTime) > 0
               ? std::string(buffer)
               : std::string();
}

} // namespace

std::string LogRecordPattern::render(const std::string& pattern,
                                     const LogRecord& record)
{
    std::string result;
    result.reserve(pattern.size() * 2);
    std::size_t index = 0;

    while (index < pattern.size()) {
        if (pattern[index] != '%') {
            result += pattern[index++];
            continue;
        }
        if (index + 1 >= pattern.size()) {
            result += '%';
            ++index;
            continue;
        }

        ++index;
        const PatternModifier modifier = parseModifier(pattern, index);
        if (index >= pattern.size()) {
            result += '%';
            break;
        }

        const char token = pattern[index++];
        if (token == '%') {
            result += '%';
            continue;
        }

        switch (token) {
        case 'm': result += applyModifier(record.message, modifier); break;
        case 'p': result += applyModifier(LoggerBase::levelToString(record.level), modifier); break;
        case 'c': result += applyModifier(record.loggerName, modifier); break;
        case 't': result += applyModifier(threadIdToString(record.threadId), modifier); break;
        case 'T': result += applyModifier(record.threadName, modifier); break;
        case 'M': result += applyModifier(joinMarkerNames(record.markers), modifier); break;
        case 'n': result += eol; break;
        case 'd': result += applyModifier(expandDate(pattern, index, record), modifier); break;
        default:
            result += '%';
            result += token;
            break;
        }
    }
    return result;
}
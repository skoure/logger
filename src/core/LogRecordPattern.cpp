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
#include <type_traits>

using namespace sk::logger;

namespace {

using Modifier = CompiledPattern::Modifier;

std::string applyModifier(std::string value, const Modifier& modifier)
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

std::string expandDate(const std::string& format, const LogRecord& record)
{
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

struct TokenRenderer
{
    const LogRecord& record;
    std::string& output;

    void operator()(const CompiledPattern::LiteralToken& token) const
    {
        output += applyModifier(token.text, token.modifier);
    }

    void operator()(const CompiledPattern::MessageToken& token) const
    {
        output += applyModifier(record.message, token.modifier);
    }

    void operator()(const CompiledPattern::LevelToken& token) const
    {
        output += applyModifier(LoggerBase::levelToString(record.level), token.modifier);
    }

    void operator()(const CompiledPattern::LoggerNameToken& token) const
    {
        output += applyModifier(record.loggerName, token.modifier);
    }

    void operator()(const CompiledPattern::ThreadIdToken& token) const
    {
        output += applyModifier(threadIdToString(record.threadId), token.modifier);
    }

    void operator()(const CompiledPattern::ThreadNameToken& token) const
    {
        output += applyModifier(record.threadName, token.modifier);
    }

    void operator()(const CompiledPattern::MarkerToken& token) const
    {
        output += applyModifier(
            renderMarkerNames(record.markers, token.options), token.modifier);
    }

    void operator()(const CompiledPattern::DateToken& token) const
    {
        output += applyModifier(expandDate(token.format, record), token.modifier);
    }

    void operator()(const CompiledPattern::NewlineToken&) const
    {
        output += eol;
    }
};

} // namespace

std::string LogRecordPattern::render(const CompiledPattern& compiledPattern,
                                     const LogRecord& record)
{
    std::string result;
    result.reserve(compiledPattern.getPattern().size() * 2);

    for (const CompiledPattern::Token& token : compiledPattern.tokens())
        std::visit(TokenRenderer{record, result}, token);
    return result;
}
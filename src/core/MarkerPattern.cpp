/**
 * @file MarkerPattern.cpp
 * @brief Implementation for handling marker patterns in the logger.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 14, 2026
 */
#include "MarkerPattern.h"
#include <logger/Exceptions.h>

#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace sk { namespace logger {

namespace {

/** @brief Trims ASCII whitespace from both ends of a string. */
std::string trim(const std::string& value)
{
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    return first < last ? std::string(first, last) : std::string();
}

/** @brief Detects a quote that wraps a `sep=` value. */
bool beginsSeparatorQuote(const std::string& text, std::size_t optionStart,
                          std::size_t index)
{
    return trim(text.substr(optionStart, index - optionStart)) == "sep=";
}

/** @brief Splits marker options at semicolons outside quoted separators. */
std::vector<std::string> splitOptions(const std::string& body)
{
    std::vector<std::string> options;
    std::size_t start = 0;
    char quote = '\0';
    for (std::size_t i = 0; i < body.size(); ++i)
    {
        const char ch = body[i];
        if (quote != '\0')
        {
            if (ch == quote)
                quote = '\0';
        }
        else if ((ch == '\'' || ch == '"') && beginsSeparatorQuote(body, start, i))
        {
            quote = ch;
        }
        else if (ch == ';')
        {
            options.push_back(body.substr(start, i - start));
            start = i + 1;
        }
    }
    if (quote != '\0')
        throw PatternLayoutParseException("Marker pattern: unmatched quote");
    options.push_back(body.substr(start));
    return options;
}

/** @brief Parses the comma-separated marker names and rejects empty entries. */
void parseMarkerList(const std::string& value, MarkerPatternOptions& options)
{
    std::size_t start = 0;
    while (start <= value.size())
    {
        const std::size_t comma = value.find(',', start);
        const std::string name = trim(value.substr(
            start, comma == std::string::npos ? std::string::npos : comma - start));
        if (name.empty())
        {
            if (!value.empty())
                throw PatternLayoutParseException("Marker pattern: empty marker name");
        }
        else if (name.compare(0, 2, "+:") == 0 || name.compare(0, 2, "-:") == 0)
        {
            throw PatternLayoutParseException(
                "Marker pattern: only one mode may be specified");
        }
        else
        {
            options.names.push_back(name);
        }

        if (comma == std::string::npos)
            break;
        start = comma + 1;
    }
}

/** @brief Parses a marker option body after its braces have been located. */
MarkerPatternOptions parseBody(const std::string& body)
{
    MarkerPatternOptions options;
    const std::vector<std::string> parts = splitOptions(body);
    const std::string markerPart = trim(parts.front());

    std::string markerNames = markerPart;
    if (markerPart.compare(0, 2, "+:") == 0)
    {
        options.mode = MarkerPatternOptions::Mode::Include;
        markerNames = markerPart.substr(2);
    }
    else if (markerPart.compare(0, 2, "-:") == 0)
    {
        options.mode = MarkerPatternOptions::Mode::Exclude;
        markerNames = markerPart.substr(2);
    }
    else if (!markerPart.empty())
    {
        options.mode = MarkerPatternOptions::Mode::Include;
    }
    parseMarkerList(markerNames, options);

    bool separatorSeen = false;
    for (std::size_t i = 1; i < parts.size(); ++i)
    {
        if (parts[i].empty())
            throw PatternLayoutParseException("Marker pattern: empty option");
        const std::size_t equals = parts[i].find('=');
        if (equals == std::string::npos)
            throw PatternLayoutParseException(
                "Marker pattern: option must use key=value syntax");

        const std::string key = trim(parts[i].substr(0, equals));
        if (key != "sep")
            throw PatternLayoutParseException("Marker pattern: unknown option '" + key + "'");
        if (separatorSeen)
            throw PatternLayoutParseException("Marker pattern: duplicate 'sep' option");
        separatorSeen = true;

        std::string value = parts[i].substr(equals + 1);
        if (!value.empty() && (value.front() == '\'' || value.front() == '"'))
        {
            if (value.size() < 2 || value.back() != value.front())
                throw PatternLayoutParseException(
                    "Marker pattern: unmatched quote in 'sep' value");
            value = value.substr(1, value.size() - 2);
        }
        else if (!value.empty() && (value.back() == '\'' || value.back() == '"'))
        {
            throw PatternLayoutParseException(
                "Marker pattern: unmatched quote in 'sep' value");
        }
        options.separator = std::move(value);
    }
    return options;
}

} // namespace

MarkerPatternOptions parseMarkerPatternOptions(const std::string& pattern,
                                               std::size_t& index)
{
    if (index >= pattern.size() || pattern[index] != '{')
        return {};

    const std::size_t start = ++index;
    std::size_t optionStart = start;
    char quote = '\0';
    for (; index < pattern.size(); ++index)
    {
        const char ch = pattern[index];
        if (quote != '\0')
        {
            if (ch == quote)
                quote = '\0';
        }
        else if ((ch == '\'' || ch == '"')
                 && beginsSeparatorQuote(pattern, optionStart, index))
        {
            quote = ch;
        }
        else if (ch == ';')
        {
            optionStart = index + 1;
        }
        else if (ch == '}')
        {
            const std::string body = pattern.substr(start, index - start);
            ++index;
            return parseBody(body);
        }
    }
    throw PatternLayoutParseException("Marker pattern: unclosed '{' in %M{...}");
}

void validateMarkerPatternOptions(const std::string& pattern)
{
    std::size_t index = 0;
    while (index < pattern.size())
    {
        if (pattern[index] != '%')
        {
            ++index;
            continue;
        }

        ++index;
        if (index >= pattern.size())
            break;
        if (pattern[index] == '%')
        {
            ++index;
            continue;
        }

        if (pattern[index] == '-')
            ++index;
        while (index < pattern.size()
               && std::isdigit(static_cast<unsigned char>(pattern[index])))
            ++index;
        if (index < pattern.size() && pattern[index] == '.')
        {
            ++index;
            while (index < pattern.size()
                   && std::isdigit(static_cast<unsigned char>(pattern[index])))
                ++index;
        }
        if (index >= pattern.size())
            break;

        const char token = pattern[index++];
        if (token == 'd' && index < pattern.size() && pattern[index] == '{')
        {
            const std::size_t close = pattern.find('}', index + 1);
            if (close == std::string::npos)
                break;
            index = close + 1;
        }
        else if (token == 'M')
        {
            parseMarkerPatternOptions(pattern, index);
        }
    }
}

std::string renderMarkerNames(const std::vector<const Marker*>& markers,
                              const MarkerPatternOptions& options)
{
    std::vector<std::string> names;
    names.reserve(markers.size());
    for (const Marker* marker : markers)
    {
        if (marker)
            names.push_back(marker->getName());
    }
    return renderMarkerNames(names, options);
}

std::string renderMarkerNames(const std::vector<std::string>& markers,
                              const MarkerPatternOptions& options)
{
    const std::unordered_set<std::string> names(options.names.begin(), options.names.end());
    std::string result;
    bool first = true;
    for (const std::string& name : markers)
    {
        const bool listed = names.find(name) != names.end();
        if ((options.mode == MarkerPatternOptions::Mode::Include && !listed)
            || (options.mode == MarkerPatternOptions::Mode::Exclude && listed))
            continue;
        if (!first)
            result += options.separator;
        result += name;
        first = false;
    }
    return result;
}

}} // namespace sk::logger
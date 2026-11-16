/**
 * @file CompiledPattern.cpp
 * @brief Compiles canonical patterns and caches marker token options.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 15, 2026
 */
#include "CompiledPattern.h"

#include <cctype>
#include <utility>

using namespace sk::logger;

namespace {

using Modifier = CompiledPattern::Modifier;

Modifier parseModifier(const std::string& pattern, std::size_t& index)
{
    Modifier modifier;
    if (index < pattern.size() && pattern[index] == '-')
    {
        modifier.leftAlign = true;
        ++index;
    }
    while (index < pattern.size()
           && std::isdigit(static_cast<unsigned char>(pattern[index])))
        modifier.minWidth = modifier.minWidth * 10 + (pattern[index++] - '0');
    if (index < pattern.size() && pattern[index] == '.')
    {
        ++index;
        modifier.maxWidth = 0;
        while (index < pattern.size()
               && std::isdigit(static_cast<unsigned char>(pattern[index])))
            modifier.maxWidth = modifier.maxWidth * 10 + (pattern[index++] - '0');
    }
    return modifier;
}

void appendLiteral(std::vector<CompiledPattern::Token>& tokens,
                   std::string text, const Modifier& modifier = {})
{
    if (!tokens.empty())
    {
        if (auto* previous = std::get_if<CompiledPattern::LiteralToken>(&tokens.back());
            previous && previous->modifier == modifier)
        {
            previous->text += text;
            return;
        }
    }
    tokens.emplace_back(CompiledPattern::LiteralToken{std::move(text), modifier});
}

void appendDateToken(std::vector<CompiledPattern::Token>& tokens,
                     const std::string& pattern, std::size_t& index,
                     const Modifier& modifier)
{
    std::string format = "%Y-%m-%d %H:%M:%S";
    if (index < pattern.size() && pattern[index] == '{')
    {
        const std::size_t close = pattern.find('}', index + 1);
        if (close == std::string::npos)
        {
            appendLiteral(tokens, "%d{", modifier);
            return;
        }
        format = pattern.substr(index + 1, close - index - 1);
        index = close + 1;
    }
    tokens.emplace_back(CompiledPattern::DateToken{modifier, std::move(format)});
}

} // namespace

CompiledPattern::CompiledPattern(std::string pattern)
    : pattern_(std::move(pattern))
{
    std::size_t index = 0;
    while (index < pattern_.size())
    {
        if (pattern_[index] != '%')
        {
            const std::size_t end = pattern_.find('%', index);
            const std::size_t count = (end == std::string::npos ? pattern_.size() : end)
                                      - index;
            appendLiteral(tokens_, pattern_.substr(index, count));
            index += count;
            continue;
        }

        ++index;
        if (index >= pattern_.size())
        {
            appendLiteral(tokens_, "%");
            continue;
        }
        if (pattern_[index] == '%')
        {
            ++index;
            appendLiteral(tokens_, "%");
            continue;
        }

        Modifier modifier = parseModifier(pattern_, index);
        if (index >= pattern_.size())
        {
            appendLiteral(tokens_, "%");
            continue;
        }

        const char token = pattern_[index++];
        switch (token)
        {
        case 'm': tokens_.emplace_back(MessageToken{modifier}); break;
        case 'p': tokens_.emplace_back(LevelToken{modifier}); break;
        case 'c': tokens_.emplace_back(LoggerNameToken{modifier}); break;
        case 't': tokens_.emplace_back(ThreadIdToken{modifier}); break;
        case 'T': tokens_.emplace_back(ThreadNameToken{modifier}); break;
        case 'M':
        {
            MarkerPatternOptions options = parseMarkerPatternOptions(pattern_, index);
            tokens_.emplace_back(MarkerToken{modifier, std::move(options)});
            break;
        }
        case 'd':
        {
            appendDateToken(tokens_, pattern_, index, modifier);
            break;
        }
        case 'n': tokens_.emplace_back(NewlineToken{}); break;
        case '%': appendLiteral(tokens_, "%"); break;
        default:
        {
            std::string text = "%";
            text += token;
            appendLiteral(tokens_, std::move(text));
            break;
        }
        }
    }
}
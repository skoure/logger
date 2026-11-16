/**
 * @file CompiledPattern.h
 * @brief Parsed canonical pattern data for repeated log-record rendering.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 15, 2026
 */
#ifndef SK_COMPILED_PATTERN_H
#define SK_COMPILED_PATTERN_H

#include "MarkerPattern.h"

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace sk { namespace logger {

class CompiledPattern
{
public:
    struct Modifier
    {
        bool leftAlign = false;
        int minWidth = 0;
        int maxWidth = -1;

        bool operator==(const Modifier& other) const noexcept
        {
            return leftAlign == other.leftAlign
                   && minWidth == other.minWidth
                   && maxWidth == other.maxWidth;
        }
    };

    struct LiteralToken
    {
        std::string text;
        Modifier modifier;
    };

    struct MessageToken { Modifier modifier; };
    struct LevelToken { Modifier modifier; };
    struct LoggerNameToken { Modifier modifier; };
    struct ThreadIdToken { Modifier modifier; };
    struct ThreadNameToken { Modifier modifier; };

    struct MarkerToken
    {
        Modifier modifier;
        MarkerPatternOptions options;
    };

    struct DateToken
    {
        Modifier modifier;
        std::string format;
    };

    struct NewlineToken {};

    using Token = std::variant<LiteralToken, MessageToken, LevelToken,
                               LoggerNameToken, ThreadIdToken, ThreadNameToken,
                               MarkerToken, DateToken, NewlineToken>;

    explicit CompiledPattern(std::string pattern);

    const std::string& getPattern() const { return pattern_; }
    const std::vector<Token>& tokens() const { return tokens_; }

private:
    std::string pattern_;
    std::vector<Token> tokens_;
};

}} // namespace sk::logger

#endif // SK_COMPILED_PATTERN_H
/**
 * @file SimpleSink.h
 * @brief Output destination and compiled pattern for SimpleLogger.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 15, 2026
 */
#ifndef SK_SIMPLE_SINK_H
#define SK_SIMPLE_SINK_H

#include <CompiledPattern.h>
#include <logger/Filter.h>
#include <logger/Logger.h>

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace sk { namespace logger {

/** @brief An output destination with its pattern compiled at construction. */
class SimpleSink
{
public:
    /**
     * @brief Construct a sink and compile its canonical pattern.
     * @param stream Output stream; the caller must keep it alive as required.
     * @param pattern Canonical log pattern, compiled during construction.
     * @param color Whether to wrap output in level-based ANSI color codes.
     * @param level Optional maximum accepted severity for this sink.
     * @param filters Filters that must all accept a record.
     */
    SimpleSink(std::shared_ptr<std::ostream> stream, std::string pattern,
               bool color = false,
               std::optional<Logger::Level> level = std::nullopt,
               std::vector<std::shared_ptr<const Filter>> filters = {});

    /** @brief Get the destination stream. */
    const std::shared_ptr<std::ostream>& getStream() const { return stream_; }
    /** @brief Return whether ANSI color output is enabled. */
    bool isColor() const { return color_; }
    /** @brief Get the sink's optional severity threshold. */
    std::optional<Logger::Level> getLevel() const { return level_; }
    /** @brief Get the filters applied to records written to this sink. */
    const std::vector<std::shared_ptr<const Filter>>& getFilters() const
    {
        return filters_;
    }
    /** @brief Get the canonical pattern used to create this sink. */
    const std::string& getPattern() const { return compiledPattern_.getPattern(); }
    /** @brief Get the pattern compiled during sink construction. */
    const CompiledPattern& getCompiledPattern() const { return compiledPattern_; }

private:
    std::shared_ptr<std::ostream> stream_;
    bool color_;
    std::optional<Logger::Level> level_;
    std::vector<std::shared_ptr<const Filter>> filters_;
    CompiledPattern compiledPattern_;
};

}} // namespace sk::logger

#endif // SK_SIMPLE_SINK_H
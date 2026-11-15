/**
 * @file SimpleSink.cpp
 * @brief SimpleSink construction and pattern compilation.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 15, 2026
 */
#include "SimpleSink.h"

#include <utility>

namespace sk::logger {

SimpleSink::SimpleSink(std::shared_ptr<std::ostream> stream, std::string pattern,
                       bool color, std::optional<Logger::Level> level,
                       std::vector<std::shared_ptr<const Filter>> filters)
    : stream_(std::move(stream)), color_(color), level_(level),
      filters_(std::move(filters)), compiledPattern_(std::move(pattern))
{
}

} // namespace sk::logger
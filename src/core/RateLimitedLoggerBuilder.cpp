/**
 * @file RateLimitedLoggerBuilder.cpp
 * @brief Implementation of RateLimitedLogger::Builder.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 22, 2026
 */
#include "RateLimitedLoggerImpl.h"
#include <stdexcept>

namespace sk { namespace logger {

RateLimitedLogger::Builder::Builder(LoggerPtr logger)
    : m_logger(std::move(logger))
{
    if (!m_logger)
    {
        throw std::invalid_argument("Logger must not be null");
    }
}

RateLimitedLogger::Builder& RateLimitedLogger::Builder::maxRate(int count)
{
    if (count <= 0)
    {
        throw std::invalid_argument("maxRate must be greater than 0");
    }
    m_maxRate = count;
    return *this;
}

RateLimitedLogger::Builder& RateLimitedLogger::Builder::every(std::chrono::nanoseconds duration)
{
    if (duration <= std::chrono::nanoseconds::zero())
    {
        throw std::invalid_argument("Window duration must be greater than 0");
    }
    m_everyDuration = duration;
    m_everyCount = 0;   // last call to every() wins
    return *this;
}

RateLimitedLogger::Builder& RateLimitedLogger::Builder::every(int count)
{
    if (count <= 0)
    {
        throw std::invalid_argument("every count must be greater than 0");
    }
    m_everyCount = count;
    m_everyDuration = std::chrono::nanoseconds::zero();   // last call to every() wins
    return *this;
}

RateLimitedLogger::Builder& RateLimitedLogger::Builder::clock(
    std::function<std::chrono::steady_clock::time_point()> clockFn)
{
    m_clock = std::move(clockFn);
    return *this;
}

RateLimitedLoggerPtr RateLimitedLogger::Builder::build()
{
    if (!m_logger)
    {
        throw std::invalid_argument("Logger must not be null");
    }
    if (m_everyDuration == std::chrono::nanoseconds::zero() && m_everyCount == 0)
    {
        throw std::invalid_argument("A rate limit policy must be configured via every()");
    }
    if (m_everyCount > 0 && m_maxRate > 0)
    {
        throw std::invalid_argument("maxRate() cannot be used with count-based rate limiting (every(count)). Did you mean every(std::chrono::duration)?");
    }

    PolicyMode mode = PolicyMode::None;
    int maxRateVal = 1;

    if (m_everyDuration > std::chrono::nanoseconds::zero())
    {
        mode = PolicyMode::TimedQuota;
        maxRateVal = m_maxRate > 0 ? m_maxRate : 1;
    }
    else if (m_everyCount > 0)
    {
        mode = PolicyMode::CountSampling;
    }

    auto impl = std::make_unique<RateLimitedLogger::Impl>(
        m_logger,
        mode,
        maxRateVal,
        m_everyDuration,
        m_everyCount,
        std::move(m_clock));

    return RateLimitedLoggerPtr(new RateLimitedLogger(std::move(impl)));
}

RateLimitedLogger::Builder RateLimitedLogger::forLogger(LoggerPtr logger)
{
    return Builder(std::move(logger));
}

}} // namespace sk::logger

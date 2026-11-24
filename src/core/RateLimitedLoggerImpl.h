/**
 * @file RateLimitedLoggerImpl.h
 * @brief Internal implementation class for RateLimitedLogger.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 22, 2026
 */
#ifndef SK_LOGGER_RATE_LIMITED_LOGGER_IMPL_H
#define SK_LOGGER_RATE_LIMITED_LOGGER_IMPL_H

#include <logger/RateLimitedLogger.h>
#include <LoggerUtils.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace sk { namespace logger {

enum class PolicyMode
{
    None,
    TimedQuota,
    CountSampling
};

class RateLimitedLogger::Impl
{
public:
    Impl(LoggerPtr logger,
         PolicyMode mode,
         int maxRate,
         std::chrono::nanoseconds windowDuration,
         int sampleInterval,
         std::function<std::chrono::steady_clock::time_point()> clockFn)
        : m_logger(std::move(logger))
        , m_mode(mode)
        , m_maxRate(maxRate)
        , m_windowDuration(windowDuration)
        , m_sampleInterval(sampleInterval)
        , m_clock(std::move(clockFn))
    {
    }

    LoggerPtr getLogger() const
    {
        return m_logger;
    }

    bool checkAdmission(int& suppressedOut)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_mode == PolicyMode::CountSampling)
        {
            m_callCount++;
            if (m_callCount == 1 || (m_callCount - 1) % m_sampleInterval == 0)
            {
                suppressedOut = m_suppressedSinceLastEmitted;
                m_suppressedSinceLastEmitted = 0;
                return true;
            }
            else
            {
                m_suppressedSinceLastEmitted++;
                return false;
            }
        }
        else if (m_mode == PolicyMode::TimedQuota)
        {
            auto now = m_clock ? m_clock() : std::chrono::steady_clock::now();

            if (!m_windowStarted)
            {
                m_windowStarted = true;
                m_windowStart = now;
                m_admittedInWindow = 1;
                suppressedOut = m_suppressedSinceLastEmitted;
                m_suppressedSinceLastEmitted = 0;
                return true;
            }

            if (now - m_windowStart >= m_windowDuration)
            {
                m_windowStart = now;
                m_admittedInWindow = 1;
                suppressedOut = m_suppressedSinceLastEmitted;
                m_suppressedSinceLastEmitted = 0;
                return true;
            }

            if (m_admittedInWindow < m_maxRate)
            {
                m_admittedInWindow++;
                suppressedOut = m_suppressedSinceLastEmitted;
                m_suppressedSinceLastEmitted = 0;
                return true;
            }
            else
            {
                m_suppressedSinceLastEmitted++;
                return false;
            }
        }

        // None / fallback
        suppressedOut = 0;
        return true;
    }

    std::string formatAndSubstitute(const char* fmt, va_list args, int suppressed) const
    {
        if (!fmt)
            return {};
        std::string fmtWithSuppressed = replaceAll(fmt, "{suppressed}", std::to_string(suppressed));
        return formatMessage(fmtWithSuppressed.c_str(), args);
    }

    std::string substituteSuppressed(const char* msg, int suppressed) const
    {
        if (!msg)
            return {};
        return replaceAll(msg, "{suppressed}", std::to_string(suppressed));
    }

    std::string substituteSuppressed(const std::string& msg, int suppressed) const
    {
        return replaceAll(msg, "{suppressed}", std::to_string(suppressed));
    }

private:
    LoggerPtr m_logger;
    PolicyMode m_mode{PolicyMode::None};
    int m_maxRate{1};
    std::chrono::nanoseconds m_windowDuration{0};
    int m_sampleInterval{1};
    std::function<std::chrono::steady_clock::time_point()> m_clock;

    mutable std::mutex m_mutex;
    bool m_windowStarted{false};
    std::chrono::steady_clock::time_point m_windowStart;
    int m_admittedInWindow{0};
    int m_suppressedSinceLastEmitted{0};
    uint64_t m_callCount{0};
};

}} // namespace sk::logger

#endif // SK_LOGGER_RATE_LIMITED_LOGGER_IMPL_H

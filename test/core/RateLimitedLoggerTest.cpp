/**
 * @file RateLimitedLoggerTest.cpp
 * @brief Unit tests for RateLimitedLogger wrapper, Builder, policies, and lazy evaluation.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 22, 2026
 */
#include <gtest/gtest.h>
#include <logger/RateLimitedLogger.h>
#include <logger/MarkerFactory.h>
#include <LoggerBase.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace sk::logger;

namespace {

/**
 * Test spy capturing records appended to it.
 */
class RateLimitSpyLogger : public LoggerBase
{
public:
    explicit RateLimitSpyLogger(const std::string& name) : m_name(name) {}

    std::string getName() const override { return m_name; }

    std::vector<LogRecord> getRecords() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_records;
    }

    std::size_t getCallCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_records.size();
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_records.clear();
    }

protected:
    void append(const LogRecord& record) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_records.push_back(record);
    }

private:
    std::string m_name;
    mutable std::mutex m_mutex;
    std::vector<LogRecord> m_records;
};

} // namespace

// ---------------------------------------------------------------------------
// Builder Validation & Option A Mutual Exclusivity
// ---------------------------------------------------------------------------

TEST(RateLimitedLoggerTest, ThrowsOnNullLogger)
{
    EXPECT_THROW(RateLimitedLogger::forLogger(nullptr), std::invalid_argument);
}

TEST(RateLimitedLoggerTest, ThrowsWhenNoPolicySpecified)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    auto builder = RateLimitedLogger::forLogger(spy);
    EXPECT_THROW(builder.build(), std::invalid_argument);
}

TEST(RateLimitedLoggerTest, ThrowsOnInvalidMaxRate)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    EXPECT_THROW(RateLimitedLogger::forLogger(spy).maxRate(0), std::invalid_argument);
    EXPECT_THROW(RateLimitedLogger::forLogger(spy).maxRate(-1), std::invalid_argument);
}

TEST(RateLimitedLoggerTest, ThrowsOnInvalidEveryCount)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    EXPECT_THROW(RateLimitedLogger::forLogger(spy).every(0), std::invalid_argument);
    EXPECT_THROW(RateLimitedLogger::forLogger(spy).every(-5), std::invalid_argument);
}

TEST(RateLimitedLoggerTest, ThrowsOnInvalidEveryDuration)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    EXPECT_THROW(RateLimitedLogger::forLogger(spy).every(std::chrono::seconds{0}), std::invalid_argument);
    EXPECT_THROW(RateLimitedLogger::forLogger(spy).every(std::chrono::seconds{-1}), std::invalid_argument);
}

TEST(RateLimitedLoggerTest, OptionAFailFastMaxRateWithCountSampling)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");

    // maxRate called before every(count)
    EXPECT_THROW({
        RateLimitedLogger::forLogger(spy)
            .maxRate(5)
            .every(10)
            .build();
    }, std::invalid_argument);

    // maxRate called after every(count)
    EXPECT_THROW({
        RateLimitedLogger::forLogger(spy)
            .every(10)
            .maxRate(5)
            .build();
    }, std::invalid_argument);
}

TEST(RateLimitedLoggerTest, LastCallToEverySetsMode)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");

    // Duration followed by count -> enters count sampling (maxRate unset, succeeds)
    EXPECT_NO_THROW({
        auto logger = RateLimitedLogger::forLogger(spy)
            .every(std::chrono::seconds{10})
            .every(5)
            .build();
        EXPECT_NE(logger, nullptr);
    });

    // Count followed by duration -> enters timed quota (succeeds)
    EXPECT_NO_THROW({
        auto logger = RateLimitedLogger::forLogger(spy)
            .every(5)
            .every(std::chrono::seconds{10})
            .maxRate(2)
            .build();
        EXPECT_NE(logger, nullptr);
    });
}

// ---------------------------------------------------------------------------
// Count Sampling: every(count)
// ---------------------------------------------------------------------------

TEST(RateLimitedLoggerTest, SamplingEvery10Emits1_11_21WithSuppressedCounts)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    spy->setLevel(Logger::Level::Trace);

    auto rateLogger = RateLimitedLogger::forLogger(spy)
        .every(10)
        .build();

    // Call 25 times
    for (int i = 1; i <= 25; ++i)
    {
        rateLogger->warn("Call %d; {suppressed} repeats", i);
    }

    auto records = spy->getRecords();
    // Emitted calls should be 1, 11, 21 -> 3 total
    ASSERT_EQ(records.size(), 3u);

    // Call 1: suppressed = 0
    EXPECT_EQ(records[0].message, "Call 1; 0 repeats");

    // Call 11: suppressed = 9 (calls 2..10)
    EXPECT_EQ(records[1].message, "Call 11; 9 repeats");

    // Call 21: suppressed = 9 (calls 12..20)
    EXPECT_EQ(records[2].message, "Call 21; 9 repeats");
}

TEST(RateLimitedLoggerTest, SamplingEvery1AdmitsAllCalls)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    spy->setLevel(Logger::Level::Trace);

    auto rateLogger = RateLimitedLogger::forLogger(spy)
        .every(1)
        .build();

    for (int i = 1; i <= 5; ++i)
    {
        rateLogger->info("Call %d ({suppressed})", i);
    }

    auto records = spy->getRecords();
    ASSERT_EQ(records.size(), 5u);
    for (int i = 0; i < 5; ++i)
    {
        EXPECT_EQ(records[i].message, "Call " + std::to_string(i + 1) + " (0)");
    }
}

// ---------------------------------------------------------------------------
// Timed Quota: every(duration) and maxRate(count) with Fake Clock
// ---------------------------------------------------------------------------

TEST(RateLimitedLoggerTest, TimedQuotaDefaultMaxRateIsOne)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    spy->setLevel(Logger::Level::Trace);

    auto fakeNow = std::chrono::steady_clock::now();
    auto clockFn = [&fakeNow]() { return fakeNow; };

    // Default maxRate = 1
    auto rateLogger = RateLimitedLogger::forLogger(spy)
        .every(std::chrono::seconds{10})
        .clock(clockFn)
        .build();

    // Call 1 at t=0s -> admitted
    rateLogger->warn("Call 1: {suppressed} suppressed");

    // Calls 2, 3, 4 at t=1s, 5s, 9s -> suppressed
    fakeNow += std::chrono::seconds{1};
    rateLogger->warn("Call 2: {suppressed} suppressed");
    fakeNow += std::chrono::seconds{4};
    rateLogger->warn("Call 3: {suppressed} suppressed");
    fakeNow += std::chrono::seconds{4};
    rateLogger->warn("Call 4: {suppressed} suppressed");

    EXPECT_EQ(spy->getCallCount(), 1u);

    // Call 5 at t=10s (window duration elapsed) -> admitted with 3 suppressed
    fakeNow += std::chrono::seconds{1}; // now t=10s
    rateLogger->warn("Call 5: {suppressed} suppressed");

    auto records = spy->getRecords();
    ASSERT_EQ(records.size(), 2u);
    EXPECT_EQ(records[0].message, "Call 1: 0 suppressed");
    EXPECT_EQ(records[1].message, "Call 5: 3 suppressed");

    // Call 6 at t=25s (long after window expired, zero calls in between) -> admitted with 0 suppressed
    fakeNow += std::chrono::seconds{15};
    rateLogger->warn("Call 6: {suppressed} suppressed");

    records = spy->getRecords();
    ASSERT_EQ(records.size(), 3u);
    EXPECT_EQ(records[2].message, "Call 6: 0 suppressed");
}

TEST(RateLimitedLoggerTest, TimedQuotaCustomMaxRateAdmitsUpToLimit)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    spy->setLevel(Logger::Level::Trace);

    auto fakeNow = std::chrono::steady_clock::now();
    auto clockFn = [&fakeNow]() { return fakeNow; };

    auto rateLogger = RateLimitedLogger::forLogger(spy)
        .maxRate(3)
        .every(std::chrono::seconds{10})
        .clock(clockFn)
        .build();

    // Calls 1, 2, 3 at t=0s -> all admitted
    rateLogger->error("Err 1 ({suppressed})");
    rateLogger->error("Err 2 ({suppressed})");
    rateLogger->error("Err 3 ({suppressed})");

    // Calls 4, 5 at t=2s -> suppressed
    fakeNow += std::chrono::seconds{2};
    rateLogger->error("Err 4 ({suppressed})");
    rateLogger->error("Err 5 ({suppressed})");

    EXPECT_EQ(spy->getCallCount(), 3u);

    // Call 6 at t=10s -> admitted (first in new window, reports 2 suppressed)
    fakeNow += std::chrono::seconds{8};
    rateLogger->error("Err 6 ({suppressed})");

    // Call 7 at t=11s -> admitted (second in new window, reports 0 suppressed)
    fakeNow += std::chrono::seconds{1};
    rateLogger->error("Err 7 ({suppressed})");

    auto records = spy->getRecords();
    ASSERT_EQ(records.size(), 5u);
    EXPECT_EQ(records[0].message, "Err 1 (0)");
    EXPECT_EQ(records[1].message, "Err 2 (0)");
    EXPECT_EQ(records[2].message, "Err 3 (0)");
    EXPECT_EQ(records[3].message, "Err 6 (2)");
    EXPECT_EQ(records[4].message, "Err 7 (0)");
}

// ---------------------------------------------------------------------------
// Placeholder Semantics
// ---------------------------------------------------------------------------

TEST(RateLimitedLoggerTest, SuppressedCountResetsEvenWithoutPlaceholder)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    spy->setLevel(Logger::Level::Trace);

    auto rateLogger = RateLimitedLogger::forLogger(spy)
        .every(3)
        .build();

    // Call 1 admitted (suppressed = 0)
    rateLogger->warn("Plain message");
    // Call 2 suppressed
    rateLogger->warn("Plain message");
    // Call 3 suppressed
    rateLogger->warn("Plain message");

    // Call 4 admitted: format string has NO placeholder!
    rateLogger->warn("Another plain message");

    // Call 5 suppressed
    rateLogger->warn("Plain message");
    // Call 6 suppressed
    rateLogger->warn("Plain message");

    // Call 7 admitted: format string HAS placeholder -> should show 2, NOT 4!
    rateLogger->warn("With placeholder: {suppressed}");

    auto records = spy->getRecords();
    ASSERT_EQ(records.size(), 3u);
    EXPECT_EQ(records[0].message, "Plain message");
    EXPECT_EQ(records[1].message, "Another plain message");
    EXPECT_EQ(records[2].message, "With placeholder: 2");
}

TEST(RateLimitedLoggerTest, MultiplePlaceholdersAndPrintfConversions)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    spy->setLevel(Logger::Level::Trace);

    auto rateLogger = RateLimitedLogger::forLogger(spy)
        .every(2)
        .build();

    rateLogger->info("Count: %d, {suppressed} repeats ({suppressed} skipped)", 100);
    rateLogger->info("Count: %d, {suppressed} repeats ({suppressed} skipped)", 101); // suppressed
    rateLogger->info("Count: %d, {suppressed} repeats ({suppressed} skipped)", 102); // admitted

    auto records = spy->getRecords();
    ASSERT_EQ(records.size(), 2u);
    EXPECT_EQ(records[0].message, "Count: 100, 0 repeats (0 skipped)");
    EXPECT_EQ(records[1].message, "Count: 102, 1 repeats (1 skipped)");
}

TEST(RateLimitedLoggerTest, ExceptionContextMessagePlaceholderSubstitution)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    spy->setLevel(Logger::Level::Trace);

    auto rateLogger = RateLimitedLogger::forLogger(spy)
        .every(2)
        .build();

    std::runtime_error ex("connection reset");
    rateLogger->error("Operation failed; {suppressed} suppressed", ex); // Call 1 admitted
    rateLogger->error("Operation failed; {suppressed} suppressed", ex); // Call 2 suppressed
    rateLogger->error("Operation failed; {suppressed} suppressed", ex); // Call 3 admitted

    auto records = spy->getRecords();
    ASSERT_EQ(records.size(), 2u);
    EXPECT_NE(records[0].message.find("Operation failed; 0 suppressed"), std::string::npos);
    EXPECT_NE(records[1].message.find("Operation failed; 1 suppressed"), std::string::npos);
    EXPECT_NE(records[1].message.find("connection reset"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Lazy Lambda Evaluation
// ---------------------------------------------------------------------------

TEST(RateLimitedLoggerTest, LazyLambdaNeverInvokedWhenLevelDisabled)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    spy->setLevel(Logger::Level::Error); // Warn is disabled

    auto rateLogger = RateLimitedLogger::forLogger(spy)
        .every(1)
        .build();

    bool invoked = false;
    rateLogger->warn([&invoked] {
        invoked = true;
        return "Should not run";
    });

    EXPECT_FALSE(invoked);
    EXPECT_EQ(spy->getCallCount(), 0u);
}

TEST(RateLimitedLoggerTest, LazyLambdaNeverInvokedWhenSuppressed)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    spy->setLevel(Logger::Level::Trace);

    auto rateLogger = RateLimitedLogger::forLogger(spy)
        .every(5)
        .build();

    int invokeCount = 0;
    for (int i = 1; i <= 10; ++i)
    {
        rateLogger->warn([&invokeCount, i] {
            ++invokeCount;
            return "Admitted call " + std::to_string(i);
        });
    }

    // Out of 10 calls with interval 5, calls 1 and 6 are admitted
    EXPECT_EQ(invokeCount, 2);
    auto records = spy->getRecords();
    ASSERT_EQ(records.size(), 2u);
    EXPECT_EQ(records[0].message, "Admitted call 1");
    EXPECT_EQ(records[1].message, "Admitted call 6");
}

TEST(RateLimitedLoggerTest, LazyLambdaWithSuppressedParameter)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    spy->setLevel(Logger::Level::Trace);

    auto rateLogger = RateLimitedLogger::forLogger(spy)
        .every(4)
        .build();

    for (int i = 1; i <= 8; ++i)
    {
        rateLogger->info([i](int suppressed) {
            return "Call " + std::to_string(i) + " after " + std::to_string(suppressed) + " drops";
        });
    }

    auto records = spy->getRecords();
    ASSERT_EQ(records.size(), 2u);
    EXPECT_EQ(records[0].message, "Call 1 after 0 drops");
    EXPECT_EQ(records[1].message, "Call 5 after 3 drops");
}

TEST(RateLimitedLoggerTest, LazyLambdaWithMarkerAndException)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    spy->setLevel(Logger::Level::Trace);

    auto marker = MarkerFactory::getMarker("NETWORK");
    auto rateLogger = RateLimitedLogger::forLogger(spy)
        .every(2)
        .build();

    std::runtime_error ex("timeout");
    int lambdaRun = 0;

    rateLogger->error(*marker, [&lambdaRun] {
        ++lambdaRun;
        return "Network timeout ({suppressed})";
    }, ex); // call 1 admitted

    rateLogger->error(*marker, [&lambdaRun] {
        ++lambdaRun;
        return "Network timeout ({suppressed})";
    }, ex); // call 2 suppressed

    EXPECT_EQ(lambdaRun, 1);
    auto records = spy->getRecords();
    ASSERT_EQ(records.size(), 1u);
    ASSERT_EQ(records[0].markers.size(), 1u);
    EXPECT_EQ(records[0].markers[0]->getName(), "NETWORK");
    EXPECT_NE(records[0].message.find("Network timeout (0)"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Logger Capability Forwarding & Markers
// ---------------------------------------------------------------------------

TEST(RateLimitedLoggerTest, LevelAndFlushControlsForwarding)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    spy->setLevel(Logger::Level::Info);

    auto rateLogger = RateLimitedLogger::forLogger(spy)
        .every(1)
        .build();

    EXPECT_EQ(rateLogger->getName(), "Test");
    EXPECT_EQ(rateLogger->getLevel(), Logger::Level::Info);
    EXPECT_TRUE(rateLogger->isInfoEnabled());
    EXPECT_FALSE(rateLogger->isDebugEnabled());

    rateLogger->setLevel(Logger::Level::Debug);
    EXPECT_EQ(spy->getLevel(), Logger::Level::Debug);
    EXPECT_TRUE(rateLogger->isDebugEnabled());

    rateLogger->setFlushOn(Logger::Level::Error);
    EXPECT_EQ(rateLogger->getFlushOn(), Logger::Level::Error);

    rateLogger->clearFlushOn();
    EXPECT_EQ(rateLogger->getFlushOn(), std::nullopt);

    rateLogger->clearLevel();
    EXPECT_FALSE(rateLogger->isLevelExplicitlySet());
}

TEST(RateLimitedLoggerTest, AllSeverityLevelsAndMarkerOverloads)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    spy->setLevel(Logger::Level::Trace);

    auto m1 = MarkerFactory::getMarker("M1");
    auto m2 = MarkerFactory::getMarker("M2");
    MarkerList mList{*m1, *m2};

    auto rateLogger = RateLimitedLogger::forLogger(spy)
        .every(1)
        .build();

    rateLogger->fatal("fatal %d", 1);
    rateLogger->error("error %d", 2);
    rateLogger->warn ("warn %d",  3);
    rateLogger->info ("info %d",  4);
    rateLogger->debug("debug %d", 5);
    rateLogger->trace("trace %d", 6);

    rateLogger->fatal(*m1, "fatal marker %d", 1);
    rateLogger->error(*m1, "error marker %d", 2);
    rateLogger->warn (*m1, "warn marker %d",  3);
    rateLogger->info (*m1, "info marker %d",  4);
    rateLogger->debug(*m1, "debug marker %d", 5);
    rateLogger->trace(*m1, "trace marker %d", 6);

    rateLogger->fatal(mList, "fatal list %d", 1);
    rateLogger->error(mList, "error list %d", 2);
    rateLogger->warn (mList, "warn list %d",  3);
    rateLogger->info (mList, "info list %d",  4);
    rateLogger->debug(mList, "debug list %d", 5);
    rateLogger->trace(mList, "trace list %d", 6);

    EXPECT_EQ(spy->getCallCount(), 18u);
}

// ---------------------------------------------------------------------------
// Thread Safety & Multi-Handle Independence
// ---------------------------------------------------------------------------

TEST(RateLimitedLoggerTest, DistinctHandlesHaveIndependentState)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    spy->setLevel(Logger::Level::Trace);

    auto handle1 = RateLimitedLogger::forLogger(spy).every(5).build();
    auto handle2 = RateLimitedLogger::forLogger(spy).every(5).build();

    // Fire handle1 4 times (1 admitted, 3 suppressed)
    for (int i = 0; i < 4; ++i)
        handle1->warn("H1");

    // Fire handle2 1 time (should be admitted, unaffected by handle1)
    handle2->warn("H2");

    auto records = spy->getRecords();
    ASSERT_EQ(records.size(), 2u);
    EXPECT_EQ(records[0].message, "H1");
    EXPECT_EQ(records[1].message, "H2");
}

TEST(RateLimitedLoggerTest, ConcurrentCallsToSingleHandle)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("ConcurrentTest");
    spy->setLevel(Logger::Level::Trace);

    constexpr int kInterval = 10;
    auto rateLogger = RateLimitedLogger::forLogger(spy)
        .every(kInterval)
        .build();

    constexpr int kNumThreads = 4;
    constexpr int kCallsPerThread = 250; // total 1000 calls
    std::vector<std::thread> threads;

    for (int t = 0; t < kNumThreads; ++t)
    {
        threads.emplace_back([&rateLogger] {
            for (int i = 0; i < kCallsPerThread; ++i)
            {
                rateLogger->info("Worker msg");
            }
        });
    }

    for (auto& th : threads)
    {
        th.join();
    }

    // 1000 total calls with interval 10 -> exactly 100 calls admitted (1, 11, 21 ... 991)
    EXPECT_EQ(spy->getCallCount(), 100u);
}

TEST(RateLimitedLoggerTest, ImplicitConversionToLoggerPtr)
{
    auto spy = std::make_shared<RateLimitSpyLogger>("Test");
    RateLimitedLoggerPtr rateLimited = RateLimitedLogger::forLogger(spy).every(1).build();

    // Verify implicit conversion to LoggerPtr
    LoggerPtr logger = rateLimited;
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->getName(), "Test");
}

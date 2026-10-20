#include <gtest/gtest.h>

#include <logger/FilterFactory.h>
#include <logger/LoggerFactory.h>
#include <logger/MarkerFactory.h>
#include <LoggerBase.h>
#include <LogRecord.h>

#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace sk::logger;

namespace {

class CountingFilter final : public Filter {
public:
    mutable int calls = 0;

    bool accepts(const LogRecord&) const override
    {
        ++calls;
        return true;
    }
};

class GlobalOstreamSinkTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        LoggerFactory::clearOstreamSinks();
    }

    void TearDown() override
    {
        LoggerFactory::clearOstreamSinks();
    }

    static void disableAdditivity(const LoggerPtr& logger)
    {
        auto base = std::dynamic_pointer_cast<LoggerBase>(logger);
        ASSERT_NE(base, nullptr);
        base->setAdditivity(false);
    }
};

} // namespace

TEST_F(GlobalOstreamSinkTest, WritesFormattedOutputFromEveryLogger)
{
    LoggerPtr logger = LoggerFactory::getLogger("GlobalOstream.Format");
    logger->setLevel(Logger::Level::Trace);

    std::ostringstream output;
    LoggerFactory::addOstreamSink(output, "[%p] [%c] %m%n");
    logger->info("hello global sink");

    EXPECT_EQ(output.str(), "[INFO] [GlobalOstream.Format] hello global sink\n");
}

TEST_F(GlobalOstreamSinkTest, ReceivesEventsFromLoggersCreatedAfterRegistration)
{
    std::ostringstream output;
    LoggerFactory::addOstreamSink(output, "%c:%m%n");

    LoggerPtr logger = LoggerFactory::getLogger("GlobalOstream.FutureLogger.Child");
    logger->setLevel(Logger::Level::Trace);
    logger->info("created after sink registration");

    EXPECT_EQ(output.str(),
              "GlobalOstream.FutureLogger.Child:created after sink registration\n");
}

TEST_F(GlobalOstreamSinkTest, FiltersNonAdditiveLoggersByNameMarkerAndMessage)
{
    LoggerPtr target = LoggerFactory::getLogger("GlobalOstream.Target");
    LoggerPtr child = LoggerFactory::getLogger("GlobalOstream.Target.Child");
    LoggerPtr other = LoggerFactory::getLogger("GlobalOstream.Other");
    for (const auto& logger : {target, child, other}) {
        logger->setLevel(Logger::Level::Trace);
        disableAdditivity(logger);
    }

    auto marker = MarkerFactory::getMarker("SECURITY");
    auto markerFilter = FilterFactory::createMarkerFilter({marker});
    auto nameFilter = FilterFactory::createLoggerNameFilter({"GlobalOstream.Target"});
    auto messageFilter = FilterFactory::createRegexFilter("timed.?out");

    std::ostringstream output;
    LoggerFactory::addOstreamSink(
        output, "%c:%M:%m%n", {markerFilter, nameFilter, messageFilter});

    target->info(*marker, "database timed out");
    target->info(*marker, "database healthy");
    target->info("database timed out");
    child->info(*marker, "database timed out");
    other->info(*marker, "database timed out");

    EXPECT_EQ(output.str(), "GlobalOstream.Target:SECURITY:database timed out\n");
}

TEST_F(GlobalOstreamSinkTest, ExcludeMarkerFilterPassesUnmarkedAndOtherEvents)
{
    LoggerPtr logger = LoggerFactory::getLogger("GlobalOstream.ExcludeMarker");
    logger->setLevel(Logger::Level::Trace);
    auto debug = MarkerFactory::getMarker("NOISY_SQL");
    auto filter = FilterFactory::createMarkerFilter({debug}, FilterMode::Exclude);

    std::ostringstream output;
    LoggerFactory::addOstreamSink(output, "%m%n", {filter});
    logger->info(*debug, "blocked");
    logger->info("unmarked");
    logger->info(*MarkerFactory::getMarker("SECURITY"), "other marker");

    EXPECT_EQ(output.str(), "unmarked\nother marker\n");
}

TEST_F(GlobalOstreamSinkTest, StopsAtFirstRejectedFilter)
{
    LoggerPtr logger = LoggerFactory::getLogger("GlobalOstream.ShortCircuit");
    logger->setLevel(Logger::Level::Trace);

    auto reject = FilterFactory::createLoggerNameFilter({"No.Such.Logger"});
    auto counting = std::make_shared<CountingFilter>();
    std::ostringstream output;
    LoggerFactory::addOstreamSink(output, "%m%n", {reject, counting});

    logger->info("not written");

    EXPECT_TRUE(output.str().empty());
    EXPECT_EQ(counting->calls, 0);
}

TEST_F(GlobalOstreamSinkTest, AppliesLevelBeforeFilters)
{
    LoggerPtr logger = LoggerFactory::getLogger("GlobalOstream.Level");
    logger->setLevel(Logger::Level::Trace);

    auto counting = std::make_shared<CountingFilter>();
    std::ostringstream warnOutput;
    std::ostringstream traceOutput;
    LoggerFactory::addOstreamSink(
        warnOutput, "%m%n", {counting}, Logger::Level::Warn);
    LoggerFactory::addOstreamSink(traceOutput, "%m%n");

    logger->info("info message");
    EXPECT_EQ(counting->calls, 0);
    logger->warn("warn message");

    EXPECT_EQ(warnOutput.str(), "warn message\n");
    EXPECT_EQ(traceOutput.str(), "info message\nwarn message\n");
    EXPECT_EQ(counting->calls, 1);
}

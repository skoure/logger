/**
 * @file FlushOnTest.cpp
 * @brief Unit tests for the flush_on severity threshold (SimpleLogger backend).
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: April 24, 2026
 */
#include <gtest/gtest.h>
#include <SimpleLogger.h>
#include <JsonConfigParser.h>
#include <sstream>

using namespace sk::logger;

// ---------------------------------------------------------------------------
// Helper: a streambuf whose sync() increments a counter each time flush()
// is called on the owning ostream.  overflow() accumulates characters so
// the buffer acts as a minimal in-memory sink.
// ---------------------------------------------------------------------------

struct FlushCountingBuf : std::streambuf
{
    int         flushCount = 0;
    std::string content;

    int overflow(int c) override
    {
        if (c != EOF) content += static_cast<char>(c);
        return c;
    }

    int sync() override
    {
        ++flushCount;
        return 0;
    }
};

// Wraps FlushCountingBuf + ostream together so both have the same lifetime.
struct CountingStream
{
    FlushCountingBuf buf;
    std::ostream     os;
    CountingStream() : os(&buf) {}
};

// Build a SimpleSink pointing at the given ostream (non-owning shared_ptr).
static SimpleSink makeSink(std::ostream& os, const std::string& pattern = "%m%n")
{
    SimpleSink s;
    s.stream  = std::shared_ptr<std::ostream>(&os, [](std::ostream*) {});
    s.pattern = pattern;
    s.color   = false;
    return s;
}

// ---------------------------------------------------------------------------
// flush_on accessor tests — pure LoggerBase state, no backend needed
// ---------------------------------------------------------------------------

TEST(FlushOnTest, DefaultNoFlushOn)
{
    SimpleLogger logger("FlushOn.Default");
    EXPECT_FALSE(logger.getFlushOn().has_value());
}

TEST(FlushOnTest, SetFlushOnStored)
{
    SimpleLogger logger("FlushOn.Set");
    logger.setFlushOn(Logger::Level::Warn);
    ASSERT_TRUE(logger.getFlushOn().has_value());
    EXPECT_EQ(*logger.getFlushOn(), Logger::Level::Warn);
}

TEST(FlushOnTest, ClearFlushOnRemovesThreshold)
{
    SimpleLogger logger("FlushOn.Clear");
    logger.setFlushOn(Logger::Level::Error);
    logger.clearFlushOn();
    EXPECT_FALSE(logger.getFlushOn().has_value());
}

// ---------------------------------------------------------------------------
// Flush-counting tests
// ---------------------------------------------------------------------------

TEST(FlushOnTest, NoFlushWhenThresholdNotSet)
{
    CountingStream cs;
    SimpleLogger logger("FlushOn.NoThreshold");
    logger.setLevel(Logger::Level::Trace);
    logger.setSinks({ makeSink(cs.os) });

    logger.fatal("fatal");
    logger.error("error");
    logger.warn ("warn");

    EXPECT_EQ(cs.buf.flushCount, 0);
}

TEST(FlushOnTest, FlushCalledWhenLevelMeetsThreshold)
{
    CountingStream cs;
    SimpleLogger logger("FlushOn.Threshold");
    logger.setLevel(Logger::Level::Trace);
    logger.setFlushOn(Logger::Level::Warn);
    logger.setSinks({ makeSink(cs.os) });

    logger.info ("info");   // below threshold — no flush
    EXPECT_EQ(cs.buf.flushCount, 0);

    logger.warn ("warn");   // meets threshold
    EXPECT_EQ(cs.buf.flushCount, 1);

    logger.error("error");  // above threshold (more severe)
    EXPECT_EQ(cs.buf.flushCount, 2);

    logger.fatal("fatal");  // above threshold
    EXPECT_EQ(cs.buf.flushCount, 3);

    logger.debug("debug");  // below threshold — no flush
    EXPECT_EQ(cs.buf.flushCount, 3);
}

TEST(FlushOnTest, ClearFlushOnDisablesFlushing)
{
    CountingStream cs;
    SimpleLogger logger("FlushOn.ClearDisables");
    logger.setLevel(Logger::Level::Trace);
    logger.setFlushOn(Logger::Level::Warn);
    logger.setSinks({ makeSink(cs.os) });

    logger.warn("before clear");
    EXPECT_EQ(cs.buf.flushCount, 1);

    logger.clearFlushOn();

    logger.warn ("after clear warn");
    logger.error("after clear error");
    logger.fatal("after clear fatal");
    EXPECT_EQ(cs.buf.flushCount, 1);  // no additional flushes
}

TEST(FlushOnTest, InheritsFlushOnFromParent)
{
    CountingStream cs;

    auto parent = std::make_shared<SimpleLogger>("FlushOn.Parent");
    parent->setLevel(Logger::Level::Trace);
    parent->setFlushOn(Logger::Level::Error);

    auto child = std::make_shared<SimpleLogger>("FlushOn.Parent.Child");
    child->setLevel(Logger::Level::Trace);
    child->setParent(parent);
    child->setSinks({ makeSink(cs.os) });

    // Child has no explicit flush_on — should inherit Error from parent.
    ASSERT_TRUE(child->getFlushOn().has_value());
    EXPECT_EQ(*child->getFlushOn(), Logger::Level::Error);

    child->warn ("warn");   // below inherited threshold
    EXPECT_EQ(cs.buf.flushCount, 0);

    child->error("error");  // meets inherited threshold
    EXPECT_EQ(cs.buf.flushCount, 1);
}

TEST(FlushOnTest, ExplicitChildFlushOnOverridesParent)
{
    CountingStream cs;

    auto parent = std::make_shared<SimpleLogger>("FlushOn.Override.Parent");
    parent->setLevel(Logger::Level::Trace);
    parent->setFlushOn(Logger::Level::Fatal);  // parent only flushes on Fatal

    auto child = std::make_shared<SimpleLogger>("FlushOn.Override.Child");
    child->setLevel(Logger::Level::Trace);
    child->setParent(parent);
    child->setFlushOn(Logger::Level::Warn);    // child explicit — overrides parent
    child->setSinks({ makeSink(cs.os) });

    child->warn("warn");
    EXPECT_EQ(cs.buf.flushCount, 1);  // flushed at Warn, not held until Fatal
}

// ---------------------------------------------------------------------------
// JSON parsing tests (these parse only — no backend involvement)
// ---------------------------------------------------------------------------

TEST(FlushOnTest, JsonParserParsesFlushOn)
{
    std::istringstream iss(R"({
        "loggers":[{"name":"root","level":"INFO","flush_on":"WARN","sinks":[]}]
    })");
    auto configs = JsonConfigParser::parse(iss);
    ASSERT_EQ(configs.size(), 1u);
    EXPECT_EQ(configs[0].flushOn, Logger::Level::Warn);
}

TEST(FlushOnTest, JsonParserMissingFlushOnIsEmpty)
{
    std::istringstream iss(R"({
        "loggers":[{"name":"root","level":"INFO","sinks":[]}]
    })");
    auto configs = JsonConfigParser::parse(iss);
    ASSERT_EQ(configs.size(), 1u);
    EXPECT_FALSE(configs[0].flushOn.has_value());
}

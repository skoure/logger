/**
 * @file LazyLoggerTest.cpp
 * @brief Unit tests for LazyLogger — the SIOF proxy logger.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: April 23, 2026
 */
#include <gtest/gtest.h>
#include <LazyLogger.h>
#include <LogRecord.h>
#include <logger/LoggerFactory.h>
#include <chrono>
#include <memory>
#include <sstream>

using namespace sk::logger;

// --- Direct LazyLogger construction tests ---
// These bypass the factory singleton so they are backend-agnostic.

TEST(LazyLoggerTest, GetNameReturnsConstructorArgument)
{
    LazyLogger logger("My.Test.Logger");
    EXPECT_EQ(logger.getName(), "My.Test.Logger");
}

TEST(LazyLoggerTest, DefaultLevelIsInfo)
{
    LazyLogger logger("LazyTest.DefaultLevel");
    EXPECT_EQ(logger.getLevel(), Logger::Level::Info);
}

TEST(LazyLoggerTest, ExplicitLevelCanBeSet)
{
    LazyLogger logger("LazyTest.SetLevel");
    logger.setLevel(Logger::Level::Debug);
    EXPECT_EQ(logger.getLevel(), Logger::Level::Debug);
    EXPECT_TRUE(logger.isLevelExplicitlySet());
}

TEST(LazyLoggerTest, ClearLevelRevertsToDefault)
{
    LazyLogger logger("LazyTest.ClearLevel");
    logger.setLevel(Logger::Level::Error);
    logger.clearLevel();
    EXPECT_EQ(logger.getLevel(), Logger::Level::Info);
    EXPECT_FALSE(logger.isLevelExplicitlySet());
}

// --- Level inheritance via parent chain ---
// LazyLogger uses LoggerBase's weak_ptr parent chain, so level inheritance
// works correctly even before the real backend logger is created.

TEST(LazyLoggerTest, InheritsParentLevelWhenNotExplicitlySet)
{
    auto parent = std::make_shared<LazyLogger>("LazyTest.Inherit.Parent");
    auto child  = std::make_shared<LazyLogger>("LazyTest.Inherit.Child");
    child->setParent(parent);

    parent->setLevel(Logger::Level::Debug);
    EXPECT_EQ(child->getLevel(), Logger::Level::Debug);
}

TEST(LazyLoggerTest, ExplicitChildLevelNotAffectedByParent)
{
    auto parent = std::make_shared<LazyLogger>("LazyTest.ExplicitChild.Parent");
    auto child  = std::make_shared<LazyLogger>("LazyTest.ExplicitChild.Child");
    child->setParent(parent);

    parent->setLevel(Logger::Level::Error);
    child->setLevel(Logger::Level::Trace);

    EXPECT_EQ(child->getLevel(), Logger::Level::Trace);
}

TEST(LazyLoggerTest, ClearLevelRevertsToParentInheritance)
{
    auto parent = std::make_shared<LazyLogger>("LazyTest.ClearReverts.Parent");
    auto child  = std::make_shared<LazyLogger>("LazyTest.ClearReverts.Child");
    child->setParent(parent);

    parent->setLevel(Logger::Level::Error);
    child->setLevel(Logger::Level::Debug);
    EXPECT_EQ(child->getLevel(), Logger::Level::Debug);

    child->clearLevel();
    EXPECT_EQ(child->getLevel(), Logger::Level::Error);
}

TEST(LazyLoggerTest, GrandchildInheritsTransitively)
{
    auto grandparent = std::make_shared<LazyLogger>("LazyTest.Trans.GP");
    auto parent      = std::make_shared<LazyLogger>("LazyTest.Trans.GP.P");
    auto child       = std::make_shared<LazyLogger>("LazyTest.Trans.GP.P.C");
    parent->setParent(grandparent);
    child->setParent(parent);

    grandparent->setLevel(Logger::Level::Warn);
    EXPECT_EQ(child->getLevel(), Logger::Level::Warn);
}

// --- Factory integration tests ---
// Verify that getLogger() never returns null (the core SIOF fix).
// By the time these tests run, the backend is registered, so these exercise
// the fast path. The SIOF slow path (LazyLogger returned) is tested above
// by constructing LazyLogger directly.

TEST(LazyLoggerTest, GetLoggerReturnsNonNull)
{
    LoggerPtr logger = LoggerFactory::getInstance().getLogger("LazyTest.NonNull");
    EXPECT_NE(logger, nullptr);
}

TEST(LazyLoggerTest, GetLoggerReturnsSameInstanceOnRepeatCall)
{
    LoggerPtr a = LoggerFactory::getInstance().getLogger("LazyTest.Cache");
    LoggerPtr b = LoggerFactory::getInstance().getLogger("LazyTest.Cache");
    EXPECT_EQ(a.get(), b.get());
}

TEST(LazyLoggerTest, GetLoggerReturnsDifferentInstancesForDifferentNames)
{
    LoggerPtr a = LoggerFactory::getInstance().getLogger("LazyTest.Diff.A");
    LoggerPtr b = LoggerFactory::getInstance().getLogger("LazyTest.Diff.B");
    EXPECT_NE(a.get(), b.get());
}

// --- append() smoke tests ---
// Verify no crash when append() is called with a backend registered.
// The LazyLogger's call_once fires, createBackendLogger() succeeds, and
// the record is forwarded to the real backend logger.

TEST(LazyLoggerTest, AppendDoesNotCrashWithBackendRegistered)
{
    LazyLogger logger("LazyTest.AppendSmoke");
    LogRecord record;
    record.level      = Logger::Level::Info;
    record.loggerName = "LazyTest.AppendSmoke";
    record.message    = "smoke test";
    record.timestamp  = std::chrono::system_clock::now();

    EXPECT_NO_THROW(logger.append(record));
}

TEST(LazyLoggerTest, AppendCanBeCalledMultipleTimes)
{
    LazyLogger logger("LazyTest.AppendMulti");
    LogRecord record;
    record.level      = Logger::Level::Debug;
    record.loggerName = "LazyTest.AppendMulti";
    record.timestamp  = std::chrono::system_clock::now();

    record.message = "first";
    EXPECT_NO_THROW(logger.append(record));

    record.message = "second";
    EXPECT_NO_THROW(logger.append(record));

    record.message = "third";
    EXPECT_NO_THROW(logger.append(record));
}

// NOTE: The cerr fallback path (backend null when append() fires) is not tested
// here because it requires temporarily nulling the singleton backend, which
// would break other tests sharing the same binary. That path can be covered
// by an integration test that runs in its own binary with a controlled factory.

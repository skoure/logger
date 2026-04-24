/**
 * @file LoggerFactoryTest.cpp
 * @brief Unit tests for LoggerFactory singleton and logger retrieval.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 08, 2025
 * @date Last modified: November 08, 2025
 */
#include <gtest/gtest.h>
#include <logger/LoggerFactory.h>

using namespace sk::logger;

TEST(LoggerFactoryTest, SingletonInstance)
{
    LoggerFactory& instance1 = LoggerFactory::getInstance();
    LoggerFactory& instance2 = LoggerFactory::getInstance();
    ASSERT_EQ(&instance1, &instance2);
}

TEST(LoggerFactoryTest, GetLoggerReturnsSameInstance)
{
    LoggerFactory& factory = LoggerFactory::getInstance();
    LoggerPtr logger1 = factory.getLogger("testLogger");
    LoggerPtr logger2 = factory.getLogger("testLogger");
    ASSERT_EQ(logger1, logger2);
}

TEST(LoggerFactoryTest, GetLoggerReturnsDifferentInstances)
{
    LoggerFactory& factory = LoggerFactory::getInstance();
    LoggerPtr logger1 = factory.getLogger("loggerA");
    LoggerPtr logger2 = factory.getLogger("loggerB");
    ASSERT_NE(logger1, logger2);
}

TEST(LoggerFactoryTest, IntermediateAncestorsCreatedForDeepLogger)
{
    // Requesting "HierRefactor.A.B.C" must auto-create "HierRefactor.A" and
    // "HierRefactor.A.B" as real loggers so the full parent chain is wired.
    LoggerPtr deep = LoggerFactory::getInstance().getLogger("HierRefactor.A.B.C");
    ASSERT_NE(deep, nullptr);

    LoggerPtr mid = LoggerFactory::getInstance().getLogger("HierRefactor.A.B");
    LoggerPtr top = LoggerFactory::getInstance().getLogger("HierRefactor.A");
    EXPECT_NE(mid, nullptr);
    EXPECT_NE(top, nullptr);
}

TEST(LoggerFactoryTest, DeepLoggerInheritsLevelThroughIntermediates)
{
    // Set root to WARN, then get a deep logger — it should see WARN via the
    // fully-wired parent chain through auto-created intermediates.
    LoggerPtr root = LoggerFactory::getInstance().getLogger("root");
    ASSERT_NE(root, nullptr);
    root->setLevel(Logger::Level::Warn);

    LoggerPtr deep = LoggerFactory::getInstance().getLogger("HierRefactor.Level.X.Y");
    ASSERT_NE(deep, nullptr);
    EXPECT_EQ(deep->getLevel(), Logger::Level::Warn);

    root->clearLevel();
}

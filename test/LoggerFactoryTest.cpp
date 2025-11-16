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

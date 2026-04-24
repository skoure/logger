/**
 * @file Log4CxxBackendTest.cpp
 * @brief Unit tests for Log4CxxBackend interface contract and factory integration.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#include <gtest/gtest.h>
#include <Log4CxxBackend.h>
#include <Log4CxxLogger.h>
#include <logger/LoggerFactory.h>
#include <logger/Logger.h>
#include <sstream>

using namespace sk::logger;

class Log4CxxBackendTest : public ::testing::Test {
protected:
    Log4CxxBackend backend;
};

TEST_F(Log4CxxBackendTest, CreateLoggerReturnsNonNull) {
    LoggerPtr logger = backend.createLogger("Log4CxxBackend.CreateTest");
    ASSERT_NE(logger, nullptr);
}

TEST_F(Log4CxxBackendTest, CreateLoggerSetsCorrectName) {
    LoggerPtr logger = backend.createLogger("Log4CxxBackend.NameTest");
    auto* concrete = dynamic_cast<Log4CxxLogger*>(logger.get());
    ASSERT_NE(concrete, nullptr);
    EXPECT_EQ(concrete->getName(), "Log4CxxBackend.NameTest");
}

TEST_F(Log4CxxBackendTest, CreateLoggerReturnsLog4CxxLoggerType) {
    LoggerPtr logger = backend.createLogger("Log4CxxBackend.TypeTest");
    EXPECT_NE(dynamic_cast<Log4CxxLogger*>(logger.get()), nullptr);
}


TEST_F(Log4CxxBackendTest, ConfigureWithOstreamWritesFormattedOutput)
{
    LoggerPtr logger = backend.createLogger("Log4Cxx.OStream.Test");
    logger->setLevel(Logger::Level::Trace);

    std::ostringstream oss;
    backend.configureLoggerWithOstream(logger, oss, "[%p] %m%n");

    logger->info("hello log4cxx");
    EXPECT_NE(oss.str().find("hello log4cxx"), std::string::npos)
        << "output: " << oss.str();
}

// Integration: factory returns valid parent and child loggers (Log4cxx owns hierarchy natively)
TEST(Log4CxxBackendFactoryTest, FactoryReturnsValidChildLogger) {
    LoggerFactory& factory = LoggerFactory::getInstance();

    LoggerPtr parent = factory.getLogger("Log4CxxBackendIT.App");
    LoggerPtr child  = factory.getLogger("Log4CxxBackendIT.App.Db");

    ASSERT_NE(parent, nullptr);
    ASSERT_NE(child, nullptr);
    EXPECT_NE(parent, child);
}

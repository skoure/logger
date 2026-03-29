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

TEST_F(Log4CxxBackendTest, SupportsNativeHierarchyReturnsTrue) {
    EXPECT_TRUE(backend.supportsNativeHierarchy());
}

TEST_F(Log4CxxBackendTest, ApplyParentSinksIsNoOp) {
    LoggerPtr parent = backend.createLogger("Log4CxxBackend.NoOp.Parent");
    LoggerPtr child  = backend.createLogger("Log4CxxBackend.NoOp.Child");
    child->setLevel(Logger::Level::Debug);

    EXPECT_NO_THROW(backend.applyParentSinks(child, parent));

    // applyParentSinks must not change the child's level
    EXPECT_EQ(child->getLevel(), Logger::Level::Debug);
}

TEST_F(Log4CxxBackendTest, ApplyParentSinksNullChildNocrash) {
    LoggerPtr parent = backend.createLogger("Log4CxxBackend.NullChild.Parent");
    EXPECT_NO_THROW(backend.applyParentSinks(nullptr, parent));
}

TEST_F(Log4CxxBackendTest, ApplyParentSinksNullParentNocrash) {
    LoggerPtr child = backend.createLogger("Log4CxxBackend.NullParent.Child");
    EXPECT_NO_THROW(backend.applyParentSinks(child, nullptr));
}

TEST_F(Log4CxxBackendTest, ApplyParentSinksBothNullNocrash) {
    EXPECT_NO_THROW(backend.applyParentSinks(nullptr, nullptr));
}

// Integration: factory returns valid parent and child loggers when supportsNativeHierarchy() = true
TEST(Log4CxxBackendFactoryTest, FactoryReturnsValidChildLogger) {
    LoggerFactory& factory = LoggerFactory::getInstance();

    LoggerPtr parent = factory.getLogger("Log4CxxBackendIT.App");
    LoggerPtr child  = factory.getLogger("Log4CxxBackendIT.App.Db");

    ASSERT_NE(parent, nullptr);
    ASSERT_NE(child, nullptr);
    EXPECT_NE(parent, child);
}

/**
 * @file SimpleLoggerBackendTest.cpp
 * @brief Unit tests for SimpleLoggerBackend interface contract and factory integration.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#include <gtest/gtest.h>
#include <SimpleLoggerBackend.h>
#include <SimpleLogger.h>
#include <logger/LoggerFactory.h>
#include <logger/Logger.h>
#include <sstream>

using namespace sk::logger;

class SimpleLoggerBackendTest : public ::testing::Test {
protected:
    SimpleLoggerBackend backend;
};

TEST_F(SimpleLoggerBackendTest, CreateLoggerReturnsNonNull) {
    LoggerPtr logger = backend.createLogger("SimpleBackend.CreateTest");
    ASSERT_NE(logger, nullptr);
}

TEST_F(SimpleLoggerBackendTest, CreateLoggerSetsCorrectName) {
    LoggerPtr logger = backend.createLogger("SimpleBackend.NameTest");
    auto* concrete = dynamic_cast<SimpleLogger*>(logger.get());
    ASSERT_NE(concrete, nullptr);
    EXPECT_EQ(concrete->getName(), "SimpleBackend.NameTest");
}

TEST_F(SimpleLoggerBackendTest, CreateLoggerReturnsSimpleLoggerType) {
    LoggerPtr logger = backend.createLogger("SimpleBackend.TypeTest");
    EXPECT_NE(dynamic_cast<SimpleLogger*>(logger.get()), nullptr);
}

TEST_F(SimpleLoggerBackendTest, SupportsNativeHierarchyReturnsFalse) {
    EXPECT_FALSE(backend.supportsNativeHierarchy());
}

TEST_F(SimpleLoggerBackendTest, ApplyParentSinksIsNoOp) {
    LoggerPtr parent = backend.createLogger("SimpleBackend.NoOp.Parent");
    LoggerPtr child  = backend.createLogger("SimpleBackend.NoOp.Child");
    child->setLevel(Logger::Level::Debug);

    EXPECT_NO_THROW(backend.applyParentSinks(child, parent));

    // applyParentSinks must not change the child's level — level copy is the factory's job
    EXPECT_EQ(child->getLevel(), Logger::Level::Debug);
}

TEST_F(SimpleLoggerBackendTest, ApplyParentSinksNullChildNocrash) {
    LoggerPtr parent = backend.createLogger("SimpleBackend.NullChild.Parent");
    EXPECT_NO_THROW(backend.applyParentSinks(nullptr, parent));
}

TEST_F(SimpleLoggerBackendTest, ApplyParentSinksNullParentNocrash) {
    LoggerPtr child = backend.createLogger("SimpleBackend.NullParent.Child");
    EXPECT_NO_THROW(backend.applyParentSinks(child, nullptr));
}

TEST_F(SimpleLoggerBackendTest, ApplyParentSinksBothNullNocrash) {
    EXPECT_NO_THROW(backend.applyParentSinks(nullptr, nullptr));
}

TEST_F(SimpleLoggerBackendTest, ConfigureWithOstreamWritesFormattedOutput)
{
    LoggerPtr logger = backend.createLogger("Simple.OStream.Test");
    logger->setLevel(Logger::Level::Trace);

    std::ostringstream oss;
    backend.configureLoggerWithOstream(logger, oss, "[%p] %m%n");

    logger->info("hello simple");
    EXPECT_NE(oss.str().find("[INFO] hello simple"), std::string::npos)
        << "output: " << oss.str();
}

// Integration: LoggerFactoryImpl copies parent level to child when supportsNativeHierarchy() = false
TEST(SimpleLoggerBackendFactoryTest, ChildInheritsParentLevel) {
    LoggerFactory& factory = LoggerFactory::getInstance();

    LoggerPtr parent = factory.getLogger("SimpleBackendIT.App");
    parent->setLevel(Logger::Level::Warn);

    LoggerPtr child = factory.getLogger("SimpleBackendIT.App.Db");
    EXPECT_EQ(child->getLevel(), Logger::Level::Warn);
}

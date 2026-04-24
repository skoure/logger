/**
 * @file SpdlogBackendTest.cpp
 * @brief Unit tests for SpdlogBackend interface contract and factory integration.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#include <gtest/gtest.h>
#include <SpdlogBackend.h>
#include <SpdlogLogger.h>
#include <spdlog/sinks/ostream_sink.h>
#include <logger/LoggerFactory.h>
#include <logger/Logger.h>
#include <sstream>

using namespace sk::logger;

class SpdlogBackendTest : public ::testing::Test {
protected:
    SpdlogBackend backend;
};

TEST_F(SpdlogBackendTest, CreateLoggerReturnsNonNull) {
    LoggerPtr logger = backend.createLogger("SpdlogBackend.CreateTest");
    ASSERT_NE(logger, nullptr);
}

TEST_F(SpdlogBackendTest, CreateLoggerSetsCorrectName) {
    LoggerPtr logger = backend.createLogger("SpdlogBackend.NameTest");
    auto* concrete = dynamic_cast<SpdlogLogger*>(logger.get());
    ASSERT_NE(concrete, nullptr);
    EXPECT_EQ(concrete->getName(), "SpdlogBackend.NameTest");
}

TEST_F(SpdlogBackendTest, CreateLoggerReturnsSpdlogLoggerType) {
    LoggerPtr logger = backend.createLogger("SpdlogBackend.TypeTest");
    EXPECT_NE(dynamic_cast<SpdlogLogger*>(logger.get()), nullptr);
}

TEST_F(SpdlogBackendTest, ImplementsManagedSinkBackend) {
    // spdlog relies on the factory for sink inheritance — must be an IManagedSinkBackend.
    EXPECT_NE(dynamic_cast<IManagedSinkBackend*>(&backend), nullptr);
}

TEST_F(SpdlogBackendTest, ApplyParentSinksCopiesSinksToChild) {
    LoggerPtr parentPtr = backend.createLogger("SpdlogBackend.SinkCopy.Parent");
    LoggerPtr childPtr  = backend.createLogger("SpdlogBackend.SinkCopy.Child");

    auto* parent = dynamic_cast<SpdlogLogger*>(parentPtr.get());
    auto* child  = dynamic_cast<SpdlogLogger*>(childPtr.get());
    ASSERT_NE(parent, nullptr);
    ASSERT_NE(child,  nullptr);

    // Add a sentinel sink to the parent's internal spdlog logger
    std::ostringstream oss;
    auto sentinel = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
    parent->getInternalLogger()->sinks().push_back(sentinel);

    backend.applyParentSinks(childPtr, parentPtr);

    auto& childSinks  = child->getInternalLogger()->sinks();
    auto& parentSinks = parent->getInternalLogger()->sinks();

    EXPECT_EQ(childSinks.size(), parentSinks.size());

    bool found = false;
    for (const auto& s : childSinks) {
        if (s == sentinel) { found = true; break; }
    }
    EXPECT_TRUE(found) << "Sentinel sink was not copied to child";
}

TEST_F(SpdlogBackendTest, ApplyParentSinksNullChildNocrash) {
    LoggerPtr parent = backend.createLogger("SpdlogBackend.NullChild.Parent");
    EXPECT_NO_THROW(backend.applyParentSinks(nullptr, parent));
}

TEST_F(SpdlogBackendTest, ApplyParentSinksNullParentNocrash) {
    LoggerPtr child = backend.createLogger("SpdlogBackend.NullParent.Child");
    EXPECT_NO_THROW(backend.applyParentSinks(child, nullptr));
}

TEST_F(SpdlogBackendTest, ApplyParentSinksBothNullNocrash) {
    EXPECT_NO_THROW(backend.applyParentSinks(nullptr, nullptr));
}

// Integration: LoggerFactoryImpl copies parent level to child (spdlog is IManagedSinkBackend)
TEST(SpdlogBackendFactoryTest, ChildInheritsParentLevel) {
    LoggerFactory& factory = LoggerFactory::getInstance();

    LoggerPtr parent = factory.getLogger("SpdlogBackendIT.App");
    parent->setLevel(Logger::Level::Debug);

    LoggerPtr child = factory.getLogger("SpdlogBackendIT.App.Db");
    EXPECT_EQ(child->getLevel(), Logger::Level::Debug);
}

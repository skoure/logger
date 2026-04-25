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
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <SinkConfig.h>
#include <logger/LoggerFactory.h>
#include <logger/Logger.h>
#include <functional>
#include <sstream>
#include <unistd.h>

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

TEST_F(SpdlogBackendTest, ConsoleSinkDefaultIsPlainSink)
{
    LoggerPtr logger = backend.createLogger("SpdlogBackend.Console.DefaultNoColor");
    auto* sl = dynamic_cast<SpdlogLogger*>(logger.get());
    ASSERT_NE(sl, nullptr);

    SinkConfig sc;
    sc.type    = "console";
    sc.pattern = "[%p] %m%n";

    backend.configureLogger(logger, {sc});

    auto& sinks = sl->getInternalLogger()->sinks();
    ASSERT_EQ(sinks.size(), 1u);
    EXPECT_NE(
        dynamic_cast<spdlog::sinks::stdout_sink_mt*>(sinks[0].get()),
        nullptr) << "Default console sink should be stdout_sink_mt";
}

TEST_F(SpdlogBackendTest, ConsoleSinkColorFalseUsesPlainSink)
{
    LoggerPtr logger = backend.createLogger("SpdlogBackend.Console.NoColor");
    auto* sl = dynamic_cast<SpdlogLogger*>(logger.get());
    ASSERT_NE(sl, nullptr);

    SinkConfig sc;
    sc.type    = "console";
    sc.pattern = "[%p] %m%n";
    sc.properties["color"] = "false";

    backend.configureLogger(logger, {sc});

    auto& sinks = sl->getInternalLogger()->sinks();
    ASSERT_EQ(sinks.size(), 1u);
    EXPECT_NE(
        dynamic_cast<spdlog::sinks::stdout_sink_mt*>(sinks[0].get()),
        nullptr) << "color=false console sink should be stdout_sink_mt";
}

// ---------------------------------------------------------------------------
// ANSI color tests
// ---------------------------------------------------------------------------

namespace {

// Redirect stdout to a pipe, run fn(), restore stdout, return captured bytes.
static std::string captureStdout(std::function<void()> fn)
{
    int pipefd[2];
    if (::pipe(pipefd) != 0) return {};

    int saved = ::dup(STDOUT_FILENO);
    if (saved < 0) { ::close(pipefd[0]); ::close(pipefd[1]); return {}; }

    ::dup2(pipefd[1], STDOUT_FILENO);
    ::close(pipefd[1]);

    fn();
    ::fflush(stdout);

    ::dup2(saved, STDOUT_FILENO);
    ::close(saved);

    char buf[512] = {};
    ssize_t n = ::read(pipefd[0], buf, sizeof(buf) - 1);
    ::close(pipefd[0]);
    return n > 0 ? std::string(buf, static_cast<std::size_t>(n)) : std::string{};
}

} // namespace

TEST_F(SpdlogBackendTest, ColorConsoleSinkEmitsGreenAnsiCodesAroundInfoLevel)
{
    // Full pipeline: translator wraps %p in %^ / %$ so stdout_color_sink_mt
    // injects ANSI codes exactly around the level name.
    // color_always=true forces ANSI output even without a TTY (test runner).
    // spdlog default for INFO: green (\x1b[32m), reset (\x1b[m).
    LoggerFactory::configureFromJsonString(R"({
        "loggers": [{
            "name": "AnsiTest.WithLevel",
            "level": "INFO",
            "sinks": [{
                "type": "console",
                "pattern": "[%p] %m%n",
                "properties": { "color": true, "color_always": true }
            }]
        }]
    })");

    LoggerPtr logger = LoggerFactory::getLogger("AnsiTest.WithLevel");
    ASSERT_NE(logger, nullptr);

    std::string output = captureStdout([&]{ logger->info("test"); });

    ASSERT_FALSE(output.empty()) << "Nothing written to stdout";
    EXPECT_NE(output.find("\x1b[32mINFO"), std::string::npos)
        << "Expected green ANSI code (\\x1b[32m) before INFO; got: " << output;
    EXPECT_NE(output.find("INFO\x1b[m"), std::string::npos)
        << "Expected ANSI reset (\\x1b[m) after INFO; got: " << output;
}

// Integration: LoggerFactoryImpl copies parent level to child (spdlog is IManagedSinkBackend)
TEST(SpdlogBackendFactoryTest, ChildInheritsParentLevel) {
    LoggerFactory& factory = LoggerFactory::getInstance();

    LoggerPtr parent = factory.getLogger("SpdlogBackendIT.App");
    parent->setLevel(Logger::Level::Debug);

    LoggerPtr child = factory.getLogger("SpdlogBackendIT.App.Db");
    EXPECT_EQ(child->getLevel(), Logger::Level::Debug);
}

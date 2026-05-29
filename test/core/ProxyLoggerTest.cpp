/**
 * @file ProxyLoggerTest.cpp
 * @brief Unit tests for ProxyLogger — the SIOF proxy logger.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: April 23, 2026
 */
#include <gtest/gtest.h>
#include <ProxyLogger.h>
#include <LogRecord.h>
#include <LoggerConfigurator.h>
#include <logger/LoggerFactory.h>
#include <chrono>
#include <memory>
#include <sstream>

using namespace sk::logger;

// --- Direct ProxyLogger construction tests ---
// These bypass the factory singleton so they are backend-agnostic.

TEST(ProxyLoggerTest, GetNameReturnsConstructorArgument)
{
    ProxyLogger logger("My.Test.Logger");
    EXPECT_EQ(logger.getName(), "My.Test.Logger");
}

TEST(ProxyLoggerTest, DefaultLevelIsInfo)
{
    ProxyLogger logger("ProxyTest.DefaultLevel");
    EXPECT_EQ(logger.getLevel(), Logger::Level::Info);
}

TEST(ProxyLoggerTest, ExplicitLevelCanBeSet)
{
    ProxyLogger logger("ProxyTest.SetLevel");
    logger.setLevel(Logger::Level::Debug);
    EXPECT_EQ(logger.getLevel(), Logger::Level::Debug);
    EXPECT_TRUE(logger.isLevelExplicitlySet());
}

TEST(ProxyLoggerTest, ClearLevelRevertsToDefault)
{
    ProxyLogger logger("ProxyTest.ClearLevel");
    logger.setLevel(Logger::Level::Error);
    logger.clearLevel();
    EXPECT_EQ(logger.getLevel(), Logger::Level::Info);
    EXPECT_FALSE(logger.isLevelExplicitlySet());
}

// --- Level inheritance via parent chain ---
// ProxyLogger uses LoggerBase's weak_ptr parent chain, so level inheritance
// works correctly even before m_real is set.

TEST(ProxyLoggerTest, InheritsParentLevelWhenNotExplicitlySet)
{
    auto parent = std::make_shared<ProxyLogger>("ProxyTest.Inherit.Parent");
    auto child  = std::make_shared<ProxyLogger>("ProxyTest.Inherit.Child");
    child->setParent(parent);

    parent->setLevel(Logger::Level::Debug);
    EXPECT_EQ(child->getLevel(), Logger::Level::Debug);
}

TEST(ProxyLoggerTest, ExplicitChildLevelNotAffectedByParent)
{
    auto parent = std::make_shared<ProxyLogger>("ProxyTest.ExplicitChild.Parent");
    auto child  = std::make_shared<ProxyLogger>("ProxyTest.ExplicitChild.Child");
    child->setParent(parent);

    parent->setLevel(Logger::Level::Error);
    child->setLevel(Logger::Level::Trace);

    EXPECT_EQ(child->getLevel(), Logger::Level::Trace);
}

TEST(ProxyLoggerTest, ClearLevelRevertsToParentInheritance)
{
    auto parent = std::make_shared<ProxyLogger>("ProxyTest.ClearReverts.Parent");
    auto child  = std::make_shared<ProxyLogger>("ProxyTest.ClearReverts.Child");
    child->setParent(parent);

    parent->setLevel(Logger::Level::Error);
    child->setLevel(Logger::Level::Debug);
    EXPECT_EQ(child->getLevel(), Logger::Level::Debug);

    child->clearLevel();
    EXPECT_EQ(child->getLevel(), Logger::Level::Error);
}

TEST(ProxyLoggerTest, GrandchildInheritsTransitively)
{
    auto grandparent = std::make_shared<ProxyLogger>("ProxyTest.Trans.GP");
    auto parent      = std::make_shared<ProxyLogger>("ProxyTest.Trans.GP.P");
    auto child       = std::make_shared<ProxyLogger>("ProxyTest.Trans.GP.P.C");
    parent->setParent(grandparent);
    child->setParent(parent);

    grandparent->setLevel(Logger::Level::Warn);
    EXPECT_EQ(child->getLevel(), Logger::Level::Warn);
}


// --- append() smoke tests ---
// Verify no crash when append() is called with a backend registered and m_real set.

TEST(ProxyLoggerTest, AppendDoesNotCrashWithRealSet)
{
    ProxyLogger logger("ProxyTest.AppendSmoke");
    // Simulate materialisation: give the proxy a real backing logger.
    auto real = std::make_shared<ProxyLogger>("ProxyTest.AppendSmoke.real");
    logger.setReal(real);

    LogRecord record;
    record.level      = Logger::Level::Info;
    record.loggerName = "ProxyTest.AppendSmoke";
    record.message    = "smoke test";
    record.timestamp  = std::chrono::system_clock::now();

    EXPECT_NO_THROW(logger.append(record));
}

TEST(ProxyLoggerTest, AppendWritesToCerrWhenNoRealSet)
{
    // Proxy with no m_real (simulates SIOF window before setBackend runs).
    ProxyLogger logger("ProxyTest.AppendCerr");
    LogRecord record;
    record.level      = Logger::Level::Warn;
    record.loggerName = "ProxyTest.AppendCerr";
    record.message    = "should go to cerr";
    record.timestamp  = std::chrono::system_clock::now();

    // Redirect cerr to capture the output; verify no crash and output is produced.
    std::ostringstream captured;
    std::streambuf* old = std::cerr.rdbuf(captured.rdbuf());
    logger.append(record);
    std::cerr.rdbuf(old);

    EXPECT_FALSE(captured.str().empty());
}

// --- setReal / getReal delegation tests ---
// Verify transparent delegation once m_real is set via setReal().

TEST(ProxyLoggerTest, GetRealReturnsNullBeforeSetReal)
{
    ProxyLogger proxy("GetReal.Null");
    EXPECT_EQ(proxy.getReal(), nullptr);
}

TEST(ProxyLoggerTest, GetRealReturnsLoggerAfterSetReal)
{
    auto proxy = std::make_shared<ProxyLogger>("GetReal.Set");
    auto real  = std::make_shared<ProxyLogger>("GetReal.Set.real");
    proxy->setReal(real);
    EXPECT_EQ(proxy->getReal(), real);
}

TEST(ProxyLoggerTest, SetRealDelegatesGetLevel)
{
    auto proxy = std::make_shared<ProxyLogger>("SetReal.Level");
    auto real  = std::make_shared<ProxyLogger>("SetReal.Level.real");
    real->setLevel(Logger::Level::Debug);

    proxy->setReal(real);

    EXPECT_EQ(proxy->getLevel(), Logger::Level::Debug);
}

TEST(ProxyLoggerTest, SetRealDelegatesSetLevel)
{
    auto proxy = std::make_shared<ProxyLogger>("SetReal.SetLevel");
    auto real  = std::make_shared<ProxyLogger>("SetReal.SetLevel.real");

    proxy->setReal(real);
    proxy->setLevel(Logger::Level::Warn);

    EXPECT_EQ(real->getLevel(), Logger::Level::Warn);
    EXPECT_EQ(proxy->getLevel(), Logger::Level::Warn);
}

TEST(ProxyLoggerTest, SetRealDelegatesClearLevel)
{
    auto proxy = std::make_shared<ProxyLogger>("SetReal.Clear");
    auto real  = std::make_shared<ProxyLogger>("SetReal.Clear.real");
    real->setLevel(Logger::Level::Error);

    proxy->setReal(real);
    proxy->clearLevel();

    EXPECT_FALSE(real->isLevelExplicitlySet());
}

TEST(ProxyLoggerTest, SetRealForwardsAccumulatedLevelToReal)
{
    auto proxy = std::make_shared<ProxyLogger>("SetReal.Forward");
    proxy->setLevel(Logger::Level::Trace);

    auto real = std::make_shared<ProxyLogger>("SetReal.Forward.real");
    proxy->setReal(real);

    EXPECT_EQ(real->getLevel(), Logger::Level::Trace);
}

// NOTE: The cerr fallback path (m_real null when append() fires) is covered by
// ProxyLoggerTest.AppendWritesToCerrWhenNoRealSet above.  The full singleton-
// backend scenario cannot be tested in a shared binary without side effects.

TEST(ProxyLoggerTest, GetLevelInheritsViaRealParentChainAfterMaterialisation)
{
    // Simulate setBackend() wiring: parent real logger has Debug explicitly set,
    // child real logger inherits via its parent pointer (as wired in setBackend).
    auto parentProxy = std::make_shared<ProxyLogger>("Chain.Parent");
    auto childProxy  = std::make_shared<ProxyLogger>("Chain.Child");
    childProxy->setParent(parentProxy);

    auto parentReal = std::make_shared<ProxyLogger>("Chain.Parent.real");
    parentReal->setLevel(Logger::Level::Debug);
    parentProxy->setReal(parentReal);

    auto childReal = std::make_shared<ProxyLogger>("Chain.Child.real");
    childReal->setParent(parentReal);
    childProxy->setReal(childReal);

    // child proxy has no explicit level; must inherit Debug via the real parent chain.
    EXPECT_EQ(childProxy->getLevel(), Logger::Level::Debug);
    EXPECT_FALSE(childProxy->isLevelExplicitlySet());
}

TEST(ProxyLoggerTest, ExplicitLevelRetainedAfterAppendWithNoReal)
{
    // Construct a ProxyLogger directly (bypasses factory, simulates a proxy in
    // the SIOF window before setBackend() materialises it). Since m_real is not
    // set, append() writes to cerr and the proxy retains its own level state.
    ProxyLogger proxy("ProxyTest.LevelRetain");
    proxy.setLevel(Logger::Level::Debug);
    ASSERT_TRUE(proxy.isLevelExplicitlySet());

    LogRecord r;
    r.level      = Logger::Level::Debug;
    r.loggerName = proxy.getName();
    r.message    = "cerr fallback — no m_real set";
    r.timestamp  = std::chrono::system_clock::now();

    std::ostringstream sink;
    std::streambuf* old = std::cerr.rdbuf(sink.rdbuf());
    proxy.append(r);
    std::cerr.rdbuf(old);

    // Level is still held on the proxy's own LoggerBase state.
    EXPECT_EQ(proxy.getLevel(), Logger::Level::Debug);
    EXPECT_TRUE(proxy.isLevelExplicitlySet());
}

TEST(ProxyLoggerTest, SiofChildHasParentAfterRootIsCreated)
{
    // Root must be auto-created so the parent chain is wired before configure.
    // We verify this by asking for a child via the factory (fast path) and
    // confirming that the child inherits a level set on root.
    LoggerPtr child = LoggerFactory::getInstance().getLogger("SiofTest.Child");
    LoggerPtr root  = LoggerFactory::getInstance().getLogger("root");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(child, nullptr);
    root->setLevel(Logger::Level::Debug);
    EXPECT_EQ(child->getLevel(), Logger::Level::Debug);
    root->clearLevel();
}

// --- SIOF end-to-end flow test ---
// Verifies that a logger obtained before configure() sees the configured level
// after configure() runs — the key observable behaviour of the SIOF fix.

TEST(SiofFlowTest, DebugEnabledAfterConfigureOnSiofLogger)
{
    // Obtain a logger (simulates static initializer code — in the test binary
    // the backend is already registered, so this goes via the fast path; the
    // observable contract is what matters).
    LoggerPtr siofLogger = LoggerFactory::getInstance().getLogger("SiofFlow.Client");
    ASSERT_NE(siofLogger, nullptr);

    // Configure root to debug — all children with no explicit level inherit it.
    LoggerConfigurator::configureFromJsonString(R"({
        "loggers": [{ "name": "root", "level": "debug" }]
    })");

    EXPECT_TRUE(siofLogger->isDebugEnabled());

    // Cleanup: restore root to default so other tests are not affected.
    LoggerFactory::getInstance().getLogger("root")->clearLevel();
}

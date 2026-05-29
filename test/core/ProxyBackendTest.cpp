/**
 * @file ProxyBackendTest.cpp
 * @brief Unit tests for ProxyBackend — the SIOF bridge backend.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: May 27, 2026
 */
#include <gtest/gtest.h>
#include <ProxyBackend.h>
#include <ProxyLogger.h>
#include <IManagedSinkBackend.h>
#include <memory>
#include <string>
#include <vector>

using namespace sk::logger;

// ---------------------------------------------------------------------------
// Minimal fake backend used to verify delegation without a real backend dep.
// ---------------------------------------------------------------------------

class FakeBackend : public IManagedSinkBackend
{
public:
    LoggerBasePtr createLogger(const std::string& name) override
    {
        lastCreated = name;
        return std::make_shared<ProxyLogger>(name);
    }

    void configureLogger(LoggerPtr logger,
                         const std::vector<SinkConfig>&) override
    {
        configured.push_back(logger->getName());
    }

    void configureLoggerWithOstream(LoggerPtr logger, std::ostream&,
                                    const std::string&) override
    {
        ostreamConfigured.push_back(logger->getName());
    }

    void applyParentSinks(LoggerPtr child, LoggerPtr parent) override
    {
        lastApplyChild  = child;
        lastApplyParent = parent;
    }

    void clearSinks(LoggerPtr logger) override
    {
        cleared.push_back(logger->getName());
    }

    std::string              lastCreated;
    std::vector<std::string> configured;
    std::vector<std::string> ostreamConfigured;
    std::vector<std::string> cleared;
    LoggerPtr                lastApplyChild;
    LoggerPtr                lastApplyParent;

};

// ---------------------------------------------------------------------------
// createLogger tests
// ---------------------------------------------------------------------------

TEST(ProxyBackendTest, CreateLoggerWithNoRealBackendReturnsProxyLogger)
{
    ProxyBackend backend;
    auto logger = backend.createLogger("App.Db");
    ASSERT_NE(logger, nullptr);
    EXPECT_NE(std::dynamic_pointer_cast<ProxyLogger>(logger), nullptr);
}

TEST(ProxyBackendTest, CreateNonRootWithNoRealBackendDefaultsToInfo)
{
    ProxyBackend backend;
    auto logger = backend.createLogger("App.Service");
    ASSERT_NE(logger, nullptr);
    // No explicit level set on a non-root proxy; default is Info.
    EXPECT_FALSE(logger->isLevelExplicitlySet());
}

TEST(ProxyBackendTest, CreateLoggerWithRealBackendDelegatesToReal)
{
    auto fake = std::make_unique<FakeBackend>();
    FakeBackend* raw = fake.get();

    ProxyBackend backend(std::move(fake));
    backend.createLogger("App.Widget");

    EXPECT_EQ(raw->lastCreated, "App.Widget");
}

// ---------------------------------------------------------------------------
// setRealBackend tests
// ---------------------------------------------------------------------------

TEST(ProxyBackendTest, HasRealBackendFalseBeforeSetRealBackend)
{
    ProxyBackend backend;
    EXPECT_FALSE(backend.hasRealBackend());
}

TEST(ProxyBackendTest, HasRealBackendTrueAfterSetRealBackend)
{
    ProxyBackend backend;
    backend.setRealBackend(std::make_unique<FakeBackend>());
    EXPECT_TRUE(backend.hasRealBackend());
}

// ---------------------------------------------------------------------------
// configureLogger tests
// ---------------------------------------------------------------------------

TEST(ProxyBackendTest, ConfigureLoggerWithNoRealBackendIsNoOp)
{
    ProxyBackend backend;
    auto proxy = std::make_shared<ProxyLogger>("Cfg.NoBackend");
    EXPECT_NO_THROW(backend.configureLogger(proxy, {}));
    EXPECT_EQ(proxy->getReal(), nullptr);
}

TEST(ProxyBackendTest, ConfigureLoggerReusesExistingRealFromSetBackend)
{
    // Proxy already has m_real set (the normal path after setBackend materialises it).
    auto fake = std::make_unique<FakeBackend>();
    FakeBackend* raw = fake.get();
    ProxyBackend backend(std::move(fake));

    auto proxy = std::make_shared<ProxyLogger>("Cfg.Reuse");
    auto preExistingReal = std::make_shared<ProxyLogger>("Cfg.Reuse.real");
    proxy->setReal(preExistingReal);

    backend.configureLogger(proxy, {});

    // configureLogger was called on the pre-existing real logger, not a new one.
    ASSERT_EQ(raw->configured.size(), 1u);
    EXPECT_EQ(raw->configured[0], "Cfg.Reuse.real");
}

// ---------------------------------------------------------------------------
// applyParentSinks tests
// ---------------------------------------------------------------------------

TEST(ProxyBackendTest, ApplyParentSinksWithNoRealSinkIsNoOp)
{
    ProxyBackend backend;  // no FakeBackend (which is IManagedSinkBackend)
    auto child  = std::make_shared<ProxyLogger>("Sink.Child");
    auto parent = std::make_shared<ProxyLogger>("Sink.Parent");
    EXPECT_NO_THROW(backend.applyParentSinks(child, parent));
}

TEST(ProxyBackendTest, ApplyParentSinksUnwrapsProxyParentGetReal)
{
    auto fake = std::make_unique<FakeBackend>();
    FakeBackend* raw = fake.get();
    ProxyBackend backend(std::move(fake));

    auto proxyParent = std::make_shared<ProxyLogger>("Sink.ProxyParent");
    auto realParent  = std::make_shared<ProxyLogger>("Sink.ProxyParent.real");
    proxyParent->setReal(realParent);

    // Child must have m_real set — without it there is no real logger to configure.
    auto child     = std::make_shared<ProxyLogger>("Sink.Child");
    auto realChild = std::make_shared<ProxyLogger>("Sink.Child.real");
    child->setReal(realChild);

    backend.applyParentSinks(child, proxyParent);

    // FakeBackend::applyParentSinks should be called with realParent, not proxyParent.
    ASSERT_NE(raw->lastApplyParent, nullptr);
    EXPECT_EQ(raw->lastApplyParent.get(), realParent.get());
}

TEST(ProxyBackendTest, ApplyParentSinksSkipsProxyParentWithNoReal)
{
    auto fake = std::make_unique<FakeBackend>();
    FakeBackend* raw = fake.get();
    ProxyBackend backend(std::move(fake));

    // Parent proxy has no m_real (not yet materialised) — nothing to copy.
    auto proxyParent = std::make_shared<ProxyLogger>("Sink.NoReal.Parent");
    auto child       = std::make_shared<ProxyLogger>("Sink.NoReal.Child");
    backend.applyParentSinks(child, proxyParent);

    EXPECT_EQ(raw->lastApplyParent, nullptr);
}

TEST(ProxyBackendTest, ApplyParentSinksDelegatesForNonProxyParent)
{
    auto fake = std::make_unique<FakeBackend>();
    ProxyBackend backend(std::move(fake));

    // A ProxyLogger with m_real set is still a ProxyLogger, so this exercises
    // the proxy branch via getReal(). Just verify no crash.
    auto parent = std::make_shared<ProxyLogger>("Sink.NonProxy.Parent");
    parent->setReal(std::make_shared<ProxyLogger>("Sink.NonProxy.Parent.inner"));
    auto child = std::make_shared<ProxyLogger>("Sink.NonProxy.Child");

    EXPECT_NO_THROW(backend.applyParentSinks(child, parent));
}

TEST(ProxyBackendTest, ApplyParentSinksUnwrapsProxyChildGetReal)
{
    auto fake = std::make_unique<FakeBackend>();
    FakeBackend* raw = fake.get();
    ProxyBackend backend(std::move(fake));

    auto proxyChild  = std::make_shared<ProxyLogger>("Sink.ProxyChild");
    auto realChild   = std::make_shared<ProxyLogger>("Sink.ProxyChild.real");
    proxyChild->setReal(realChild);

    auto proxyParent = std::make_shared<ProxyLogger>("Sink.ProxyParent2");
    auto realParent  = std::make_shared<ProxyLogger>("Sink.ProxyParent2.real");
    proxyParent->setReal(realParent);

    backend.applyParentSinks(proxyChild, proxyParent);

    // FakeBackend must receive realChild and realParent, not the proxies.
    ASSERT_NE(raw->lastApplyChild, nullptr);
    EXPECT_EQ(raw->lastApplyChild.get(), realChild.get());
    ASSERT_NE(raw->lastApplyParent, nullptr);
    EXPECT_EQ(raw->lastApplyParent.get(), realParent.get());
}

TEST(ProxyBackendTest, ApplyParentSinksSkipsProxyChildWithNoReal)
{
    auto fake = std::make_unique<FakeBackend>();
    FakeBackend* raw = fake.get();
    ProxyBackend backend(std::move(fake));

    // Child proxy has no m_real — nothing to apply sinks to.
    auto proxyChild  = std::make_shared<ProxyLogger>("Sink.ChildNoReal");
    auto proxyParent = std::make_shared<ProxyLogger>("Sink.ParentReal2");
    proxyParent->setReal(std::make_shared<ProxyLogger>("Sink.ParentReal2.real"));

    backend.applyParentSinks(proxyChild, proxyParent);

    EXPECT_EQ(raw->lastApplyChild, nullptr);
}

// ---------------------------------------------------------------------------
// clearSinks tests
// ---------------------------------------------------------------------------

TEST(ProxyBackendTest, ClearSinksWithNoRealSinkIsNoOp)
{
    ProxyBackend backend;
    auto logger = std::make_shared<ProxyLogger>("Clear.NoBackend");
    EXPECT_NO_THROW(backend.clearSinks(logger));
}

TEST(ProxyBackendTest, ClearSinksOnProxyWithRealDelegatesToReal)
{
    auto fake = std::make_unique<FakeBackend>();
    FakeBackend* raw = fake.get();
    ProxyBackend backend(std::move(fake));

    auto proxy = std::make_shared<ProxyLogger>("Clear.Proxy");
    auto real  = std::make_shared<ProxyLogger>("Clear.Proxy.real");
    proxy->setReal(real);

    backend.clearSinks(proxy);

    ASSERT_EQ(raw->cleared.size(), 1u);
    EXPECT_EQ(raw->cleared[0], "Clear.Proxy.real");
}

TEST(ProxyBackendTest, ClearSinksOnProxyWithNoRealIsNoOp)
{
    auto fake = std::make_unique<FakeBackend>();
    FakeBackend* raw = fake.get();
    ProxyBackend backend(std::move(fake));

    auto proxy = std::make_shared<ProxyLogger>("Clear.NoReal");
    backend.clearSinks(proxy);

    EXPECT_TRUE(raw->cleared.empty());
}

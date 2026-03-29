/**
 * @file LoggerHierarchyTest.cpp
 * @brief Unit tests for hierarchical logger support.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 15, 2025
 */
#include <gtest/gtest.h>
#include <LoggerHierarchy.h>
#include <logger/LoggerFactory.h>
#include <memory>

using namespace sk::logger;

class LoggerHierarchyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear hierarchy before each test
        hierarchy.clear();
    }

    LoggerHierarchy hierarchy;
};

// Test hierarchy creation
TEST_F(LoggerHierarchyTest, CreateRootLogger) {
    auto logger = LoggerFactory::getInstance().getLogger("root");
    hierarchy.addLogger("root", logger);

    EXPECT_EQ(hierarchy.getRoot(), logger);
    EXPECT_EQ(hierarchy.getLogger("root"), logger);
}

// Test adding loggers to hierarchy
TEST_F(LoggerHierarchyTest, AddLoggersToHierarchy) {
    auto root = LoggerFactory::getInstance().getLogger("root");
    auto app = LoggerFactory::getInstance().getLogger("App");
    auto db = LoggerFactory::getInstance().getLogger("App.Database");

    hierarchy.addLogger("root", root);
    hierarchy.addLogger("App", app);
    hierarchy.addLogger("App.Database", db);

    EXPECT_EQ(hierarchy.getLogger("root"), root);
    EXPECT_EQ(hierarchy.getLogger("App"), app);
    EXPECT_EQ(hierarchy.getLogger("App.Database"), db);
}

// Test parent-child relationships
TEST_F(LoggerHierarchyTest, ParentChildRelationships) {
    auto root = LoggerFactory::getInstance().getLogger("root");
    auto app = LoggerFactory::getInstance().getLogger("App");
    auto db = LoggerFactory::getInstance().getLogger("App.Database");

    hierarchy.addLogger("root", root);
    hierarchy.addLogger("App", app);
    hierarchy.addLogger("App.Database", db);

    // Check parent relationships
    EXPECT_EQ(hierarchy.getParent("App"), root);
    EXPECT_EQ(hierarchy.getParent("App.Database"), app);
    EXPECT_EQ(hierarchy.getParent("root"), nullptr);
}

// Test children retrieval
TEST_F(LoggerHierarchyTest, GetChildren) {
    auto root = LoggerFactory::getInstance().getLogger("root");
    auto app = LoggerFactory::getInstance().getLogger("App");
    auto db = LoggerFactory::getInstance().getLogger("App.Database");
    auto sql = LoggerFactory::getInstance().getLogger("App.Database.SQL");

    hierarchy.addLogger("root", root);
    hierarchy.addLogger("App", app);
    hierarchy.addLogger("App.Database", db);
    hierarchy.addLogger("App.Database.SQL", sql);

    // Root should have App as child
    auto rootChildren = hierarchy.getChildren("root");
    ASSERT_EQ(rootChildren.size(), 1);
    EXPECT_EQ(rootChildren[0], app);

    // App should have App.Database as child
    auto appChildren = hierarchy.getChildren("App");
    ASSERT_EQ(appChildren.size(), 1);
    EXPECT_EQ(appChildren[0], db);

    // App.Database should have App.Database.SQL as child
    auto dbChildren = hierarchy.getChildren("App.Database");
    ASSERT_EQ(dbChildren.size(), 1);
    EXPECT_EQ(dbChildren[0], sql);
}


// Test deep hierarchy
TEST_F(LoggerHierarchyTest, DeepHierarchy) {
    auto root = LoggerFactory::getInstance().getLogger("root");
    auto level1 = LoggerFactory::getInstance().getLogger("L1");
    auto level2 = LoggerFactory::getInstance().getLogger("L1.L2");
    auto level3 = LoggerFactory::getInstance().getLogger("L1.L2.L3");
    auto level4 = LoggerFactory::getInstance().getLogger("L1.L2.L3.L4");

    hierarchy.addLogger("root", root);
    hierarchy.addLogger("L1", level1);
    hierarchy.addLogger("L1.L2", level2);
    hierarchy.addLogger("L1.L2.L3", level3);
    hierarchy.addLogger("L1.L2.L3.L4", level4);

    // Check logger count
    EXPECT_EQ(hierarchy.getLoggerCount(), 5);

    // Check parent chain
    EXPECT_EQ(hierarchy.getParent("L1"), root);
    EXPECT_EQ(hierarchy.getParent("L1.L2"), level1);
    EXPECT_EQ(hierarchy.getParent("L1.L2.L3"), level2);
    EXPECT_EQ(hierarchy.getParent("L1.L2.L3.L4"), level3);
}

// Test automatic parent creation
TEST_F(LoggerHierarchyTest, AutomaticParentCreation) {
    auto root = LoggerFactory::getInstance().getLogger("root");
    auto sql = LoggerFactory::getInstance().getLogger("App.Database.SQL");

    hierarchy.addLogger("root", root);
    hierarchy.addLogger("App.Database.SQL", sql);

    // Intermediate nodes should be created
    EXPECT_TRUE(hierarchy.getLoggerNode("App") != nullptr);
    EXPECT_TRUE(hierarchy.getLoggerNode("App.Database") != nullptr);

    // Intermediate nodes should have no logger instance but should be in hierarchy
    EXPECT_EQ(hierarchy.getLogger("App.Database.SQL"), sql);
}

// Test clearing hierarchy
TEST_F(LoggerHierarchyTest, ClearHierarchy) {
    auto root = LoggerFactory::getInstance().getLogger("root");
    auto app = LoggerFactory::getInstance().getLogger("App");

    hierarchy.addLogger("root", root);
    hierarchy.addLogger("App", app);

    EXPECT_EQ(hierarchy.getLoggerCount(), 2);

    hierarchy.clear();

    EXPECT_EQ(hierarchy.getLoggerCount(), 1); // Only root node exists
    EXPECT_EQ(hierarchy.getLogger("App"), nullptr);
}

// Test multiple children at same level
TEST_F(LoggerHierarchyTest, MultipleSiblings) {
    auto root = LoggerFactory::getInstance().getLogger("root");
    auto app = LoggerFactory::getInstance().getLogger("App");
    auto system = LoggerFactory::getInstance().getLogger("System");
    auto network = LoggerFactory::getInstance().getLogger("Network");

    hierarchy.addLogger("root", root);
    hierarchy.addLogger("App", app);
    hierarchy.addLogger("System", system);
    hierarchy.addLogger("Network", network);

    auto rootChildren = hierarchy.getChildren("root");
    EXPECT_EQ(rootChildren.size(), 3);
}

// Test hierarchical configuration application
TEST_F(LoggerHierarchyTest, HierarchicalConfiguration) {
    // Test that loggers created via getLogger are hierarchy-aware
    auto rootLogger = LoggerFactory::getLogger("root");
    auto appLogger = LoggerFactory::getLogger("App");
    auto dbLogger = LoggerFactory::getLogger("App.Database");

    EXPECT_NE(rootLogger, nullptr);
    EXPECT_NE(appLogger, nullptr);
    EXPECT_NE(dbLogger, nullptr);

    // Verify subsequent calls return the same instances
    auto rootLogger2 = LoggerFactory::getLogger("root");
    auto appLogger2 = LoggerFactory::getLogger("App");

    EXPECT_EQ(rootLogger, rootLogger2);
    EXPECT_EQ(appLogger, appLogger2);
}

// ---------------------------------------------------------------------------
// Level inheritance tests — run against every backend via the shared file
// ---------------------------------------------------------------------------

TEST_F(LoggerHierarchyTest, ChildInheritsParentLevel) {
    auto parent = LoggerFactory::getLogger("LvlApp");
    auto child  = LoggerFactory::getLogger("LvlApp.Module");

    parent->setLevel(Logger::Level::Warn);

    EXPECT_FALSE(child->isLevelExplicitlySet());
    EXPECT_EQ(child->getLevel(), Logger::Level::Warn);
}

TEST_F(LoggerHierarchyTest, GrandchildInheritsTransitively) {
    auto root   = LoggerFactory::getLogger("LvlApp2");
    auto middle = LoggerFactory::getLogger("LvlApp2.Sub");
    auto leaf   = LoggerFactory::getLogger("LvlApp2.Sub.Deep");

    root->setLevel(Logger::Level::Error);

    EXPECT_EQ(middle->getLevel(), Logger::Level::Error);
    EXPECT_EQ(leaf->getLevel(),   Logger::Level::Error);
}

TEST_F(LoggerHierarchyTest, ExplicitChildNotAffectedByParentChange) {
    auto parent = LoggerFactory::getLogger("LvlApp3");
    auto child  = LoggerFactory::getLogger("LvlApp3.Module");

    child->setLevel(Logger::Level::Debug);
    parent->setLevel(Logger::Level::Error);

    EXPECT_EQ(child->getLevel(), Logger::Level::Debug);
    EXPECT_TRUE(child->isLevelExplicitlySet());
}

TEST_F(LoggerHierarchyTest, ClearLevelRevertsToParentInheritance) {
    auto parent = LoggerFactory::getLogger("LvlApp4");
    auto child  = LoggerFactory::getLogger("LvlApp4.Module");

    parent->setLevel(Logger::Level::Error);
    child->setLevel(Logger::Level::Debug);
    child->clearLevel();

    EXPECT_FALSE(child->isLevelExplicitlySet());
    EXPECT_EQ(child->getLevel(), Logger::Level::Error);
}

TEST_F(LoggerHierarchyTest, IntermediateExplicitLevelBlocksGrandparent) {
    auto root   = LoggerFactory::getLogger("LvlApp5");
    auto middle = LoggerFactory::getLogger("LvlApp5.Sub");
    auto leaf   = LoggerFactory::getLogger("LvlApp5.Sub.Deep");

    root->setLevel(Logger::Level::Error);
    middle->setLevel(Logger::Level::Debug);

    EXPECT_EQ(leaf->getLevel(), Logger::Level::Debug);
}


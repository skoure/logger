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
    hierarchy.addLogger("root", logger, true);

    EXPECT_EQ(hierarchy.getRoot(), logger);
    EXPECT_EQ(hierarchy.getLogger("root"), logger);
}

// Test adding loggers to hierarchy
TEST_F(LoggerHierarchyTest, AddLoggersToHierarchy) {
    auto root = LoggerFactory::getInstance().getLogger("root");
    auto app = LoggerFactory::getInstance().getLogger("App");
    auto db = LoggerFactory::getInstance().getLogger("App.Database");

    hierarchy.addLogger("root", root, true);
    hierarchy.addLogger("App", app, true);
    hierarchy.addLogger("App.Database", db, true);

    EXPECT_EQ(hierarchy.getLogger("root"), root);
    EXPECT_EQ(hierarchy.getLogger("App"), app);
    EXPECT_EQ(hierarchy.getLogger("App.Database"), db);
}

// Test parent-child relationships
TEST_F(LoggerHierarchyTest, ParentChildRelationships) {
    auto root = LoggerFactory::getInstance().getLogger("root");
    auto app = LoggerFactory::getInstance().getLogger("App");
    auto db = LoggerFactory::getInstance().getLogger("App.Database");

    hierarchy.addLogger("root", root, true);
    hierarchy.addLogger("App", app, true);
    hierarchy.addLogger("App.Database", db, true);

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

    hierarchy.addLogger("root", root, true);
    hierarchy.addLogger("App", app, true);
    hierarchy.addLogger("App.Database", db, true);
    hierarchy.addLogger("App.Database.SQL", sql, true);

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

// Test additivity flag
TEST_F(LoggerHierarchyTest, AdditivityFlag) {
    auto app = LoggerFactory::getInstance().getLogger("App");
    
    hierarchy.addLogger("App", app, true);
    EXPECT_TRUE(hierarchy.hasAdditivity("App"));

    hierarchy.setAdditivity("App", false);
    EXPECT_FALSE(hierarchy.hasAdditivity("App"));

    hierarchy.setAdditivity("App", true);
    EXPECT_TRUE(hierarchy.hasAdditivity("App"));
}

// Test deep hierarchy
TEST_F(LoggerHierarchyTest, DeepHierarchy) {
    auto root = LoggerFactory::getInstance().getLogger("root");
    auto level1 = LoggerFactory::getInstance().getLogger("L1");
    auto level2 = LoggerFactory::getInstance().getLogger("L1.L2");
    auto level3 = LoggerFactory::getInstance().getLogger("L1.L2.L3");
    auto level4 = LoggerFactory::getInstance().getLogger("L1.L2.L3.L4");

    hierarchy.addLogger("root", root, true);
    hierarchy.addLogger("L1", level1, true);
    hierarchy.addLogger("L1.L2", level2, true);
    hierarchy.addLogger("L1.L2.L3", level3, true);
    hierarchy.addLogger("L1.L2.L3.L4", level4, true);

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

    hierarchy.addLogger("root", root, true);
    hierarchy.addLogger("App.Database.SQL", sql, true);

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

    hierarchy.addLogger("root", root, true);
    hierarchy.addLogger("App", app, true);

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

    hierarchy.addLogger("root", root, true);
    hierarchy.addLogger("App", app, true);
    hierarchy.addLogger("System", system, true);
    hierarchy.addLogger("Network", network, true);

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


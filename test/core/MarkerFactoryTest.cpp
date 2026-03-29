/**
 * @file MarkerFactoryTest.cpp
 * @brief Unit tests for hierarchical logger support.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 15, 2025
 */
#include "gtest/gtest.h"
#include <logger/MarkerFactory.h> // Includes Marker.h indirectly
#include <sstream>
#include <memory>
#include <string>

using namespace sk::logger;

/**
 * @brief Test fixture for Marker and MarkerFactory components.
 */
class MarkerFactoryTest : public ::testing::Test {
protected:
    // Any common setup or shared variables can go here
};

// --- Test 1: Basic Marker Properties and Interface ---
TEST_F(MarkerFactoryTest, BasicProperties) {
    // Arrange: Create a marker using the factory
    const std::string markerName = "CORE_SETUP";
    std::shared_ptr<Marker> marker = MarkerFactory::getMarker(markerName);

    // Assert 1: The returned object is not null
    ASSERT_NE(marker, nullptr);

    // Assert 2: The name property is correct
    EXPECT_EQ(marker->getName(), markerName);
}

// --- Test 2: Uniqueness and Registry Logic ---
TEST_F(MarkerFactoryTest, MarkerUniqueness) {
    const std::string uniqueName = "PERFORMANCE_CRITICAL";

    // Arrange: Get the marker twice using the same name
    std::shared_ptr<Marker> markerA = MarkerFactory::getMarker(uniqueName);
    std::shared_ptr<Marker> markerB = MarkerFactory::getMarker(uniqueName);

    // Assert 1: The memory addresses MUST be the same (verifies uniqueness)
    EXPECT_EQ(markerA, markerB) 
        << "MarkerFactory failed to return the same unique instance for name: " << uniqueName;

    // Arrange: Get a third marker with a different name
    std::shared_ptr<Marker> markerC = MarkerFactory::getMarker("NETWORK_IO");

    // Assert 2: The different marker MUST have a different memory address
    EXPECT_NE(markerA, markerC) 
        << "MarkerFactory incorrectly returned the same instance for different names.";
}

// --- Test 3: Stream Insertion Operator Overload ---
TEST_F(MarkerFactoryTest, StreamOperatorOutput) {
    const std::string markerName = "DB_QUERY";
    std::shared_ptr<Marker> marker = MarkerFactory::getMarker(markerName);

    // Arrange: Use a stringstream to capture the output of the operator<<
    std::stringstream ss;
    ss << *marker; // Dereference the shared_ptr to pass the IMarker&

    // Assert: The stream output should exactly match the marker's name
    EXPECT_EQ(ss.str(), markerName) 
        << "Stream operator output did not match the expected marker name.";
}

// --- Test 4: Handling Edge Cases (Empty Name) ---
TEST_F(MarkerFactoryTest, EmptyNameHandling) {
    const std::string emptyName = "";
    
    // Act: Get a marker with an empty name
    std::shared_ptr<Marker> emptyMarker1 = MarkerFactory::getMarker(emptyName);
    std::shared_ptr<Marker> emptyMarker2 = MarkerFactory::getMarker(emptyName);

    // Assert 1: The marker should still be created
    ASSERT_NE(emptyMarker1, nullptr);

    // Assert 2: The name should be empty
    EXPECT_EQ(emptyMarker1->getName(), emptyName);

    // Assert 3: Uniqueness still holds for the empty string key
    EXPECT_EQ(emptyMarker1, emptyMarker2);
}

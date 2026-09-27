/**
 * @file MarkerListTest.cpp
 * @brief Unit tests for MarkerList.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: September 20, 2026
 */
#include <gtest/gtest.h>
#include <logger/MarkerFactory.h>
#include <logger/MarkerList.h>

using namespace sk::logger;

TEST(MarkerListTest, EmptyListIsEmpty)
{
    MarkerList ml;
    EXPECT_TRUE(ml.empty());
    EXPECT_EQ(ml.size(), 0u);
}

TEST(MarkerListTest, ConstructFromRefs)
{
    auto a = MarkerFactory::getMarker("A");
    auto b = MarkerFactory::getMarker("B");
    MarkerList ml({*a, *b});

    EXPECT_FALSE(ml.empty());
    EXPECT_EQ(ml.size(), 2u);
    EXPECT_EQ(ml.get()[0]->getName(), "A");
    EXPECT_EQ(ml.get()[1]->getName(), "B");
}

TEST(MarkerListTest, ConstructFromPointers)
{
    auto a = MarkerFactory::getMarker("A");
    auto b = MarkerFactory::getMarker("B");
    MarkerList ml({a.get(), b.get()});

    EXPECT_EQ(ml.size(), 2u);
    EXPECT_EQ(ml.get()[0]->getName(), "A");
    EXPECT_EQ(ml.get()[1]->getName(), "B");
}

TEST(MarkerListTest, OrderPreserved)
{
    auto first  = MarkerFactory::getMarker("First");
    auto second = MarkerFactory::getMarker("Second");
    auto third  = MarkerFactory::getMarker("Third");
    MarkerList ml({*third, *first, *second});

    EXPECT_EQ(ml.get()[0]->getName(), "Third");
    EXPECT_EQ(ml.get()[1]->getName(), "First");
    EXPECT_EQ(ml.get()[2]->getName(), "Second");
}

TEST(MarkerListTest, SingleElement)
{
    auto a = MarkerFactory::getMarker("A");
    MarkerList ml({*a});

    EXPECT_EQ(ml.size(), 1u);
    EXPECT_EQ(ml.get()[0]->getName(), "A");
}

TEST(MarkerListTest, WorksWithSharedPtr)
{
    auto a = MarkerFactory::getMarker("A");
    std::shared_ptr<Marker> b = MarkerFactory::getMarker("B");
    MarkerList ml({*a, *b});

    EXPECT_EQ(ml.size(), 2u);
    EXPECT_EQ(ml.get()[1]->getName(), "B");
}

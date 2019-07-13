#include "allocator.hpp"
#include <gtest/gtest.h>

TEST (Allocator, InitialState) {
    allocator a;
    EXPECT_EQ (a.num_allocs (), 0U);
    EXPECT_EQ (a.num_frees (), 0U);
}

TEST (Allocator, SimpleAllocateThenFree) {
    allocator a;
    auto p1 = a.allocate (16);
    EXPECT_EQ (a.num_allocs (), 1U);
    EXPECT_EQ (a.num_frees (), 0U);
    a.free (p1);
    EXPECT_EQ (a.num_allocs (), 0U);
    EXPECT_EQ (a.num_frees (), 1U);
}

TEST (Allocator, AllocFreeAlloc) {
    allocator a;
    auto p1 = a.allocate (16);
    a.free (p1);
    auto p2 = a.allocate (16);
    a.free (p2);
    EXPECT_EQ (p1, p2);
    EXPECT_EQ (a.num_allocs (), 0U);
    EXPECT_EQ (a.num_frees (), 1U);
}

TEST (Allocator, AllocFreeAllocLarger) {
    allocator a;
    auto p1 = a.allocate (16);
    a.free (p1);
    auto p2 = a.allocate (32);
    a.free (p2);
    EXPECT_NE (p1, p2);
    EXPECT_EQ (a.num_allocs (), 0U);
    EXPECT_EQ (a.num_frees (), 1U);
}

TEST (Allocator, SplitFreeSpace) {
    allocator a;
    auto p1 = a.allocate (32);
    auto p2 = a.allocate (16);
    a.free (p1);
    auto p3 = a.allocate (16);
    EXPECT_EQ (p3, p1);
    EXPECT_EQ (a.num_allocs (), 2U);
    EXPECT_EQ (a.num_frees (), 1U);
    a.free (p2);
    a.free (p3);
    EXPECT_EQ (a.num_allocs (), 0U);
    EXPECT_EQ (a.num_frees (), 1U);
}

TEST (Allocator, Free2) {
    allocator a;
    auto p1 = a.allocate (16);
    auto p2 = a.allocate (16);
    auto p3 = a.allocate (16);
    EXPECT_EQ (a.num_allocs (), 3U);
    EXPECT_EQ (a.num_frees (), 0U);
    a.free (p2);
    a.free (p3);
    EXPECT_EQ (a.num_allocs (), 1U);
    EXPECT_EQ (a.num_frees (), 1U);
    a.free (p1);
    EXPECT_EQ (a.num_allocs (), 0U);
    EXPECT_EQ (a.num_frees (), 1U);
}

TEST (Allocator, FreeInReverseOrder) {
    allocator a;

    auto p1 = a.allocate (16);
    auto p2 = a.allocate (16);
    auto p3 = a.allocate (16);

    a.free (p3);
    EXPECT_EQ (a.num_allocs (), 2U);
    EXPECT_EQ (a.num_frees (), 1U);

    a.free (p2);
    EXPECT_EQ (a.num_allocs (), 1U);
    EXPECT_EQ (a.num_frees (), 1U);

    a.free (p1);
    EXPECT_EQ (a.num_allocs (), 0U);
    EXPECT_EQ (a.num_frees (), 1U);
}

TEST (Allocator, FreeInForwardOrder) {
    allocator a;

    auto p1 = a.allocate (16);
    EXPECT_EQ (a.num_allocs (), 1U);
    EXPECT_EQ (a.num_frees (), 0U);
    auto p2 = a.allocate (16);
    EXPECT_EQ (a.num_allocs (), 2U);
    EXPECT_EQ (a.num_frees (), 0U);
    auto p3 = a.allocate (16);
    EXPECT_EQ (a.num_allocs (), 3U);
    EXPECT_EQ (a.num_frees (), 0U);

    a.free (p1);
    EXPECT_EQ (a.num_allocs (), 2U);
    EXPECT_EQ (a.num_frees (), 1U);
    a.free (p3);
    EXPECT_EQ (a.num_allocs (), 1U);
    EXPECT_EQ (a.num_frees (), 2U);
    a.free (p2);
    EXPECT_EQ (a.num_allocs (), 0U);
    EXPECT_EQ (a.num_frees (), 1U);
}

#include "allocator.hpp"
#include <gtest/gtest.h>

TEST (Allocator, AllocFreeAlloc) {
    allocator a;
    auto p1 = a.allocate (16);
    a.free (p1);
    auto p2 = a.allocate (16);
    a.free (p2);
    EXPECT_EQ (p1, p2);
    EXPECT_EQ (a.num_allocs (), 0);
    EXPECT_EQ (a.num_frees (), 1);
}

TEST (Allocator, AllocFreeAllocLarger) {
    allocator a;
    auto p1 = a.allocate (16);
    a.free (p1);
    auto p2 = a.allocate (32);
    a.free (p2);
    EXPECT_NE (p1, p2);
    EXPECT_EQ (a.num_allocs (), 0);
    EXPECT_EQ (a.num_frees (), 1);
}

TEST (Allocator, Free1) {
    allocator a;
    auto p1 = a.allocate (16);
    EXPECT_EQ (a.num_allocs (), 1);
    EXPECT_EQ (a.num_frees (), 0);
    a.free (p1);
    EXPECT_EQ (a.num_allocs (), 0);
    EXPECT_EQ (a.num_frees (), 1);
}

TEST (Allocator, Free2) {
    allocator a;
    auto p1 = a.allocate (16);
    auto p2 = a.allocate (16);
    auto p3 = a.allocate (16);
    EXPECT_EQ (a.num_allocs (), 3);
    EXPECT_EQ (a.num_frees (), 0);
    a.free (p2);
    a.free (p3);
    EXPECT_EQ (a.num_allocs (), 1);
    EXPECT_EQ (a.num_frees (), 1);
    a.free (p1);
    EXPECT_EQ (a.num_allocs (), 0);
    EXPECT_EQ (a.num_frees (), 1);
}

TEST (Allocator, Free3) {
    allocator a;

    auto p1 = a.allocate (16);
    EXPECT_EQ (a.num_allocs (), 1);
    EXPECT_EQ (a.num_frees (), 0);
    auto p2 = a.allocate (16);
    EXPECT_EQ (a.num_allocs (), 2);
    EXPECT_EQ (a.num_frees (), 0);
    auto p3 = a.allocate (16);
    EXPECT_EQ (a.num_allocs (), 3);
    EXPECT_EQ (a.num_frees (), 0);

    a.free (p1);
    EXPECT_EQ (a.num_allocs (), 2);
    EXPECT_EQ (a.num_frees (), 1);
    a.free (p3);
    EXPECT_EQ (a.num_allocs (), 1);
    EXPECT_EQ (a.num_frees (), 2);
    a.free (p2);
    EXPECT_EQ (a.num_allocs (), 0);
    EXPECT_EQ (a.num_frees (), 1);
}



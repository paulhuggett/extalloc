#include "allocator.hpp"
#include <gtest/gtest.h>

#include <list>
#include <vector>

namespace {

    class Allocator : public ::testing::Test {
    public:
        Allocator ();

        static constexpr std::size_t buffer_size = 256;

        std::list<std::vector<std::uint8_t>> buffers_;
        allocator alloc_;
    };

    Allocator::Allocator ()
            : alloc_{[this](std::size_t size) {
                auto & buffer = buffers_.emplace_back (std::max (size, buffer_size));
                return std::pair<uint8_t *, size_t>{buffer.data (), buffer.size ()};
            }} {}

} // end anonymous namespace


TEST_F (Allocator, InitialState) {
    EXPECT_EQ (alloc_.num_allocs (), 0U);
    EXPECT_EQ (alloc_.num_frees (), 0U);
}

TEST_F (Allocator, BadFree) {
    auto v = std::uint8_t{0};
    EXPECT_THROW (alloc_.free (&v), std::runtime_error);
}

TEST_F (Allocator, SimpleAllocateThenFree) {
    auto p1 = alloc_.allocate (16);
    EXPECT_EQ (alloc_.num_allocs (), 1U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
    alloc_.free (p1);
    EXPECT_EQ (alloc_.num_allocs (), 0U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, AllocFreeAlloc) {
    auto p1 = alloc_.allocate (16);
    alloc_.free (p1);
    auto p2 = alloc_.allocate (16);
    alloc_.free (p2);
    EXPECT_EQ (p1, p2);
    EXPECT_EQ (alloc_.num_allocs (), 0U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, AllocFreeAllocLarger) {
    auto p1 = alloc_.allocate (16);
    alloc_.free (p1);
    auto p2 = alloc_.allocate (32);
    alloc_.free (p2);
    EXPECT_EQ (p1, p2);
    EXPECT_EQ (alloc_.num_allocs (), 0U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, SplitFreeSpace) {
    auto p1 = alloc_.allocate (32);
    auto p2 = alloc_.allocate (16);
    alloc_.free (p1);
    auto p3 = alloc_.allocate (16);
    EXPECT_EQ (p3, p1);
    EXPECT_EQ (alloc_.num_allocs (), 2U);
    EXPECT_EQ (alloc_.num_frees (), 2U);
    alloc_.free (p2);
    alloc_.free (p3);
    EXPECT_EQ (alloc_.num_allocs (), 0U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, Free2) {
    auto p1 = alloc_.allocate (16);
    auto p2 = alloc_.allocate (16);
    auto p3 = alloc_.allocate (16);
    EXPECT_EQ (alloc_.num_allocs (), 3U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
    alloc_.free (p2);
    alloc_.free (p3);
    EXPECT_EQ (alloc_.num_allocs (), 1U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
    alloc_.free (p1);
    EXPECT_EQ (alloc_.num_allocs (), 0U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, FreeInReverseOrder) {
    auto p1 = alloc_.allocate (16);
    auto p2 = alloc_.allocate (16);
    auto p3 = alloc_.allocate (16);

    alloc_.free (p3);
    EXPECT_EQ (alloc_.num_allocs (), 2U);
    EXPECT_EQ (alloc_.num_frees (), 1U);

    alloc_.free (p2);
    EXPECT_EQ (alloc_.num_allocs (), 1U);
    EXPECT_EQ (alloc_.num_frees (), 1U);

    alloc_.free (p1);
    EXPECT_EQ (alloc_.num_allocs (), 0U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, FreeInForwardOrder) {
    auto p1 = alloc_.allocate (16);
    EXPECT_EQ (alloc_.num_allocs (), 1U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
    auto p2 = alloc_.allocate (16);
    EXPECT_EQ (alloc_.num_allocs (), 2U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
    auto p3 = alloc_.allocate (16);
    EXPECT_EQ (alloc_.num_allocs (), 3U);
    EXPECT_EQ (alloc_.num_frees (), 1U);

    alloc_.free (p1);
    EXPECT_EQ (alloc_.num_allocs (), 2U);
    EXPECT_EQ (alloc_.num_frees (), 2U);
    alloc_.free (p3);
    EXPECT_EQ (alloc_.num_allocs (), 1U);
    EXPECT_EQ (alloc_.num_frees (), 2U);
    alloc_.free (p2);
    EXPECT_EQ (alloc_.num_allocs (), 0U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, GrowStorageTwice) {
    auto p1 = alloc_.allocate (16);
    auto p2 = alloc_.allocate (buffer_size);
    EXPECT_EQ (buffers_.size (), 2U);
    alloc_.free (p1);
    alloc_.free (p2);
}

TEST_F (Allocator, ReallocSameSize) {
    auto p1 = alloc_.allocate (16);
    auto p2 = alloc_.realloc (p1, 16);
    EXPECT_EQ (p1, p2);
    EXPECT_EQ (alloc_.num_allocs (), 1U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, ReallocSmallerNoFollowingFreeSpace) {
    auto p1 = alloc_.allocate (buffer_size);
    auto p2 = alloc_.realloc (p1, 8);
    EXPECT_EQ (p1, p2);
    EXPECT_EQ (alloc_.num_allocs (), 1U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, ReallocSmallerWithFollowingFreeSpace) {
    auto p1 = alloc_.allocate (16);
    auto p2 = alloc_.allocate (16);
    alloc_.free (p2);
    auto p3 = alloc_.realloc (p1, 8);
    EXPECT_EQ (p1, p3);
    EXPECT_EQ (alloc_.num_allocs (), 1U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, ReallocLargerWithFollowingFreeSpace) {
    auto p1 = alloc_.allocate (8);
    auto p2 = alloc_.realloc (p1, 16);
    EXPECT_EQ (p1, p2);
    EXPECT_EQ (alloc_.num_allocs (), 1U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, ReallocLargerWithoutFollowingFreeSpace) {
    auto p1 = alloc_.allocate (8);
    auto p2 = alloc_.allocate (8);
    auto p3 = alloc_.realloc (p1, 16);
    EXPECT_NE (p1, p2);
    EXPECT_NE (p1, p3);
    EXPECT_EQ (alloc_.num_allocs (), 2U);
    EXPECT_EQ (alloc_.num_frees (), 2U);
}

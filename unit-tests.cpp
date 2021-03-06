#include "allocator.hpp"
#include <gtest/gtest.h>

#include <list>
#include <vector>

using namespace extalloc;

namespace {

    class Allocator : public ::testing::Test {
    public:
        Allocator ();

        static constexpr std::size_t buffer_size = 256;

        std::list<std::vector<std::uint8_t>> buffers_;
        allocator alloc_;
    };

    constexpr std::size_t Allocator::buffer_size;

    Allocator::Allocator ()
            : alloc_{[this](std::size_t size) {
                         buffers_.emplace_back (std::max (size, buffer_size));
                         auto & buffer = buffers_.back ();
                         return std::pair<uint8_t *, size_t>{buffer.data (), buffer.size ()};
                     },
                     std::make_pair (nullptr, std::size_t{0})} {}

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
    ASSERT_TRUE (alloc_.check ());
    EXPECT_EQ (alloc_.num_allocs (), 1U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
    alloc_.free (p1);
    ASSERT_TRUE (alloc_.check ());
    EXPECT_EQ (alloc_.num_allocs (), 0U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, AllocFreeAlloc) {
    auto p1 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    alloc_.free (p1);
    ASSERT_TRUE (alloc_.check ());

    auto p2 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    alloc_.free (p2);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (p1, p2);
    EXPECT_EQ (alloc_.num_allocs (), 0U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, AllocFreeAllocLarger) {
    auto p1 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    alloc_.free (p1);
    ASSERT_TRUE (alloc_.check ());

    auto p2 = alloc_.allocate (32);
    ASSERT_TRUE (alloc_.check ());

    alloc_.free (p2);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (p1, p2);
    EXPECT_EQ (alloc_.num_allocs (), 0U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, SplitFreeSpace) {
    auto p1 = alloc_.allocate (32);
    ASSERT_TRUE (alloc_.check ());

    auto p2 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    alloc_.free (p1);
    ASSERT_TRUE (alloc_.check ());

    auto p3 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (p3, p1);
    EXPECT_EQ (alloc_.num_allocs (), 2U);
    EXPECT_EQ (alloc_.num_frees (), 2U);

    alloc_.free (p2);
    ASSERT_TRUE (alloc_.check ());

    alloc_.free (p3);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (alloc_.num_allocs (), 0U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, Free2) {
    auto p1 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    auto p2 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    auto p3 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (alloc_.num_allocs (), 3U);
    EXPECT_EQ (alloc_.num_frees (), 1U);

    alloc_.free (p2);
    ASSERT_TRUE (alloc_.check ());

    alloc_.free (p3);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (alloc_.num_allocs (), 1U);
    EXPECT_EQ (alloc_.num_frees (), 1U);

    alloc_.free (p1);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (alloc_.num_allocs (), 0U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, FreeInReverseOrder) {
    auto p1 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    auto p2 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    auto p3 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    alloc_.free (p3);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (alloc_.num_allocs (), 2U);
    EXPECT_EQ (alloc_.num_frees (), 1U);

    alloc_.free (p2);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (alloc_.num_allocs (), 1U);
    ASSERT_TRUE (alloc_.check ());
    EXPECT_EQ (alloc_.num_frees (), 1U);

    alloc_.free (p1);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (alloc_.num_allocs (), 0U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, FreeInForwardOrder) {
    auto p1 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (alloc_.num_allocs (), 1U);
    EXPECT_EQ (alloc_.num_frees (), 1U);

    auto p2 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (alloc_.num_allocs (), 2U);
    EXPECT_EQ (alloc_.num_frees (), 1U);

    auto p3 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (alloc_.num_allocs (), 3U);
    EXPECT_EQ (alloc_.num_frees (), 1U);

    alloc_.free (p1);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (alloc_.num_allocs (), 2U);
    EXPECT_EQ (alloc_.num_frees (), 2U);

    alloc_.free (p3);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (alloc_.num_allocs (), 1U);
    EXPECT_EQ (alloc_.num_frees (), 2U);

    alloc_.free (p2);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (alloc_.num_allocs (), 0U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, GrowStorageTwice) {
    auto p1 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    auto p2 = alloc_.allocate (buffer_size);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (buffers_.size (), 2U);

    alloc_.free (p1);
    ASSERT_TRUE (alloc_.check ());

    alloc_.free (p2);
    ASSERT_TRUE (alloc_.check ());
}

TEST_F (Allocator, ReallocSameSize) {
    auto p1 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    auto p2 = alloc_.realloc (p1, 16);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (p1, p2);
    EXPECT_EQ (alloc_.num_allocs (), 1U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, ReallocSmallerNoFollowingFreeSpace) {
    auto p1 = alloc_.allocate (buffer_size);
    ASSERT_TRUE (alloc_.check ());

    auto p2 = alloc_.realloc (p1, 8);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (p1, p2);
    EXPECT_EQ (alloc_.num_allocs (), 1U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, ReallocSmallerWithFollowingFreeSpace) {
    auto p1 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    auto p2 = alloc_.allocate (16);
    ASSERT_TRUE (alloc_.check ());

    alloc_.free (p2);
    ASSERT_TRUE (alloc_.check ());

    auto p3 = alloc_.realloc (p1, 8);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (p1, p3);
    EXPECT_EQ (alloc_.num_allocs (), 1U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, ReallocLargerWithFollowingFreeSpace) {
    auto p1 = alloc_.allocate (8);
    ASSERT_TRUE (alloc_.check ());

    auto p2 = alloc_.realloc (p1, 16);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_EQ (p1, p2);
    EXPECT_EQ (alloc_.num_allocs (), 1U);
    EXPECT_EQ (alloc_.num_frees (), 1U);
}

TEST_F (Allocator, ReallocLargerWithoutFollowingFreeSpace) {
    auto p1 = alloc_.allocate (8);
    auto p2 = alloc_.allocate (8);

    ASSERT_TRUE (alloc_.check ());
    auto p3 = alloc_.realloc (p1, 16);
    ASSERT_TRUE (alloc_.check ());

    EXPECT_NE (p1, p2);
    EXPECT_NE (p1, p3);
    EXPECT_EQ (alloc_.num_allocs (), 2U);
    EXPECT_EQ (alloc_.num_frees (), 2U);
}

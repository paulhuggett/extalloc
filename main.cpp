#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <optional>
#include <random>

#include "allocator.hpp"

namespace {

    void test_alloc_free_alloc () {
        allocator a;
        auto p1 = a.allocate (16);
        a.free (p1);
        auto p2 = a.allocate (16);
        a.free (p2);
        assert (p1 == p2);
        assert (a.num_allocs () == 0);
        assert (a.num_frees () == 1);
    }

    void test_alloc_free_alloc_larger () {
        allocator a;
        auto p1 = a.allocate (16);
        a.free (p1);
        auto p2 = a.allocate (32);
        a.free (p2);
        assert (p1 != p2);
        assert (a.num_allocs () == 0);
        assert (a.num_frees () == 1);
    }

    void free1 () {
        allocator a;
        auto p1 = a.allocate (16);
        assert (a.num_allocs () == 1);
        assert (a.num_frees () == 0);
        a.free (p1);
        assert (a.num_allocs () == 0);
        assert (a.num_frees () == 1);
    }

    void free2 () {
        allocator a;
        auto p1 = a.allocate (16);
        auto p2 = a.allocate (16);
        auto p3 = a.allocate (16);
        assert (a.num_allocs () == 3);
        assert (a.num_frees () == 0);
        a.free (p2);
        a.free (p3);
        assert (a.num_allocs () == 1);
        assert (a.num_frees () == 1);
        a.free (p1);
        assert (a.num_allocs () == 0);
        assert (a.num_frees () == 1);
    }

    void free3 () {
        allocator a;

        auto p1 = a.allocate (16);
        assert (a.num_allocs () == 1);
        assert (a.num_frees () == 0);
        auto p2 = a.allocate (16);
        assert (a.num_allocs () == 2);
        assert (a.num_frees () == 0);
        auto p3 = a.allocate (16);
        assert (a.num_allocs () == 3);
        assert (a.num_frees () == 0);

        a.free (p1);
        assert (a.num_allocs () == 2);
        assert (a.num_frees () == 1);
        a.free (p3);
        assert (a.num_allocs () == 1);
        assert (a.num_frees () == 2);
        a.free (p2);
        assert (a.num_allocs () == 0);
        assert (a.num_frees () == 1);
    }


    void free_n (allocator & a, std::deque<allocator::address> & allocs, std::size_t n) {
        for (; n > 0; --n) {
            auto const pos = allocs.front ();
            allocs.pop_front ();

            a.free (pos);
        }
    }

    void stress () {
        allocator alloc;
        std::mt19937 random;

        constexpr auto num_passes = 2000U;
        constexpr auto num_allocations = 2000U;
        constexpr auto max_allocation_size = 256U;

        std::deque<allocator::address> blocks;
        for (auto pass = 0U; pass < num_passes; ++pass) {
            while (blocks.size () < num_allocations) {
                blocks.push_back (alloc.allocate (random () % max_allocation_size));
            }
            std::shuffle (blocks.begin (), blocks.end (), random);
            free_n (alloc, blocks, random () % num_allocations);
        }

        free_n (alloc, blocks, blocks.size ());
        alloc.dump (std::cout);
        assert (alloc.num_allocs () == 0);
        assert (alloc.num_frees () == 1);
    }

} // end anonymous namespace

int main () {
    int exit_code = EXIT_SUCCESS;
    try {
        test_alloc_free_alloc ();
        test_alloc_free_alloc_larger ();
        free1 ();
        free2 ();
        free3 ();
        stress ();
    } catch (std::exception const & ex) {
        std::cerr << "Error: " << ex.what () << '\n';
        exit_code = EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown error\n";
        exit_code = EXIT_FAILURE;
    }
    return exit_code;
}

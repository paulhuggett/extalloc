#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <optional>
#include <random>

#include "allocator.hpp"

namespace {

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

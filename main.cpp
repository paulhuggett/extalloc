#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <random>

#include "allocator.hpp"

namespace {

    void stress (unsigned num_passes, unsigned num_allocations, unsigned max_allocation_size) {
        allocator alloc;
        std::deque<allocator::address> blocks;

        auto const free_n = [&](std::size_t n) {
            for (; n > 0; --n) {
                auto const pos = blocks.front ();
                blocks.pop_front ();

                alloc.free (pos);
            }
        };

        std::mt19937 random;

        for (; num_passes > 0; --num_passes) {
            while (blocks.size () < num_allocations) {
                blocks.push_back (alloc.allocate (random () % max_allocation_size));
            }
            std::shuffle (blocks.begin (), blocks.end (), random);
            free_n (random () % blocks.size ());
        }

        free_n (blocks.size ());
        alloc.dump (std::cout);
        assert (alloc.num_allocs () == 0);
        assert (alloc.num_frees () == 1);
    }

} // end anonymous namespace

int main () {
    int exit_code = EXIT_SUCCESS;
    try {
        constexpr auto num_passes = 2000U;
        constexpr auto num_allocations = 2000U;
        constexpr auto max_allocation_size = 256U;

        stress (num_passes, num_allocations, max_allocation_size);
    } catch (std::exception const & ex) {
        std::cerr << "Error: " << ex.what () << '\n';
        exit_code = EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown error\n";
        exit_code = EXIT_FAILURE;
    }
    return exit_code;
}

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <deque>
#include <list>
#include <iostream>
#include <random>
#include <tuple>

#include "allocator.hpp"

namespace {

    class bad_memory : public std::runtime_error {
    public:
        explicit bad_memory ()
                : std::runtime_error{"bad allocation contents"} {}
    };

    void stress (unsigned num_passes, unsigned num_allocations, std::size_t max_allocation_size,
                 std::size_t storage_block_size) {
        std::list<std::vector<std::uint8_t>> buffers;

        allocator alloc{[&buffers, storage_block_size](std::size_t size) {
                            buffers.emplace_back (std::max (size, storage_block_size));
                            auto & buffer = buffers.back ();
                            return std::pair<uint8_t *, size_t>{buffer.data (), buffer.size ()};
                        },
                        std::make_pair (nullptr, std::size_t{0})};

        std::deque<std::tuple<allocator::address, std::size_t, std::uint8_t>> blocks;

        auto const free_n = [&alloc, &blocks](std::size_t n) {
            for (; n > 0; --n) {
                auto const & front = blocks.front ();
                allocator::address const addr = std::get<0> (front);
                std::size_t const size = std::get<1> (front);
                std::uint8_t const v = std::get<2> (front);

                auto const value = v;
                auto end = addr + size;
                if (std::find_if (addr, end, [value](std::uint8_t v) { return v != value; }) !=
                    end) {
                    throw bad_memory ();
                }

                alloc.free (addr);
                assert (alloc.check ());

                blocks.pop_front ();
            }
        };

        std::mt19937 random;

        std::cout << "Allocate checks: ";
        for (unsigned pass = 0; pass < num_passes; ++pass) {
            std::cout << '.' << std::flush;
            while (blocks.size () < num_allocations) {
                auto const size = random () % max_allocation_size;
                auto const ptr = alloc.allocate (size);
                assert (alloc.check ());
                auto const value = static_cast<std::uint8_t> (random () % 0xFF);
                std::fill_n (ptr, size, value);
                blocks.push_back (std::make_tuple (ptr, size, value));
            }
            std::shuffle (blocks.begin (), blocks.end (), random);
            free_n (random () % blocks.size ());
        }
        std::cout << std::endl << "Realloc checks: ";

        // Now the same again but doing a realloc on the block immediately after it has been
        // allocated.
        for (unsigned pass = 0; pass < num_passes; ++pass) {
            std::cout << '.' << std::flush;
            while (blocks.size () < num_allocations) {
                auto const ptr = alloc.allocate (random () % max_allocation_size);
                assert (alloc.check ());

                auto const size = random () % max_allocation_size;
                auto const ptr2 = alloc.realloc (ptr, size);
                assert (alloc.check ());

                auto const value = static_cast<std::uint8_t> (random () % 0xFF);
                std::fill_n (ptr2, size, value);
                blocks.push_back (std::make_tuple (ptr2, size, value));
            }
            std::shuffle (blocks.begin (), blocks.end (), random);
            free_n (random () % blocks.size ());
        }
        std::cout << std::endl;

        free_n (blocks.size ());

        alloc.dump (std::cout);
        assert (alloc.num_allocs () == 0);
    }

} // end anonymous namespace

int main () {
    int exit_code = EXIT_SUCCESS;
    try {
        constexpr auto num_passes = 16U;
        constexpr auto num_allocations = 2000U;
        constexpr auto max_allocation_size = std::size_t{256};
        constexpr auto storage_block_size = std::size_t{32768};

        stress (num_passes, num_allocations, max_allocation_size, storage_block_size);
    } catch (std::exception const & ex) {
        std::cerr << "Error: " << ex.what () << '\n';
        exit_code = EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown error\n";
        exit_code = EXIT_FAILURE;
    }
    return exit_code;
}

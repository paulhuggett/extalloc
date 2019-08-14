#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <system_error>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "allocator.hpp"

namespace {




    class bad_memory : public std::runtime_error {
    public:
        explicit bad_memory ()
                : std::runtime_error{"bad allocation contents"} {}
    };

    using blocks_type = std::map<allocator::address, std::pair<std::size_t, std::uint8_t>>;

    bool block_content_okay (blocks_type::value_type const & vt) {
        auto const addr = vt.first;
        auto const & [size, v] = vt.second;
        auto const value = v;
        auto end = addr + size;
        return std::find_if (addr, end, [value](std::uint8_t v) { return v != value; }) == end;
    }

    bool blocks_okay (blocks_type const & blocks, allocator & alloc) {
        if (blocks.size () != alloc.num_allocs ()) {
            return false;
        }

        if (!std::equal (
                std::begin (blocks), std::end (blocks), alloc.allocs_begin (),
                [](blocks_type::value_type const & v1,
                   allocator::container::value_type const & v2) { return v1.first == v2.first; })) {
            return false;
        }

        if (!std::equal (std::begin (blocks), std::end (blocks), alloc.allocs_begin (),
                         [](blocks_type::value_type const & v1,
                            allocator::container::value_type const & v2) {
                             return std::get<0> (v1.second) <= v2.second;
                         })) {
            return false;
        }
        if (!std::all_of (std::begin (blocks), std::end (blocks), block_content_okay)) {
            return false;
        }
        return true;
    }


    void free_n (std::mt19937 & random, blocks_type * const blocks, allocator * const alloc) {
        if (blocks->size () == 0) {
            return;
        }

        for (std::size_t n = random () % blocks->size (); n > 0; --n) {
            auto pos = blocks->begin ();
            std::advance (pos, random () % blocks->size ());

            if (!blocks_okay (*blocks, *alloc)) {
                throw bad_memory ();
            }

            auto const addr = pos->first;
            auto const end = addr + std::get<0> (pos->second);
            auto const value = std::get<1> (pos->second);
            if (std::find_if (addr, end, [value](std::uint8_t v) { return v != value; }) != end) {
                throw bad_memory ();
            }

            alloc->free (addr);
            assert (alloc->check ());

            blocks->erase (pos);
            if (!blocks_okay (*blocks, *alloc)) {
                throw bad_memory ();
            }
        }
    }

    void allocate_test (unsigned num_passes, unsigned num_allocations,
                        std::size_t max_allocation_size, std::mt19937 & random,
                        blocks_type & blocks, allocator * const alloc) {
        std::cout << "Allocate checks: ";

        for (auto pass = 0U; pass < num_passes; ++pass) {
            std::cout << '.' << std::flush;

            while (alloc->num_allocs () < num_allocations) {
                if (!blocks_okay (blocks, *alloc)) {
                    throw bad_memory ();
                }

                auto const size = random () % max_allocation_size;
                auto const ptr = alloc->allocate (size);
                assert (alloc->check ());

                auto const value = static_cast<std::uint8_t> ((random () % 26) + 'a');
                std::fill_n (ptr, size, value);
                blocks[ptr] = std::make_pair (size, value);
            }

            free_n (random, &blocks, alloc);
        }
        std::cout << std::endl;
    }


    void realloc_test (unsigned num_passes, unsigned num_allocations,
                       std::size_t max_allocation_size, std::mt19937 & random, blocks_type & blocks,
                       allocator * const alloc) {
        std::cout << "Realloc checks: ";

        // Now the same again but doing a realloc on the block immediately after it has been
        // allocated.
        for (auto pass = 0U; pass < num_passes; ++pass) {
            std::cout << '.' << std::flush;

            while (alloc->num_allocs () < num_allocations) {
                if (!blocks_okay (blocks, *alloc)) {
                    throw bad_memory ();
                }

                auto const ptr = alloc->allocate (random () % max_allocation_size);
                assert (alloc->check ());

                auto const size = random () % max_allocation_size;
                auto const ptr2 = alloc->realloc (ptr, size);
                assert (alloc->check ());

                auto const value = static_cast<std::uint8_t> ((random () % 26) + 'a');
                std::fill_n (ptr2, size, value);
                blocks[ptr2] = std::make_pair (size, value);
            }

            if (blocks.size () > 0) {
                free_n (random, &blocks, alloc);
            }
        }
        std::cout << std::endl;
    }


    class deleter {
    public:
        explicit deleter (std::size_t mapped_size) noexcept
                : mapped_size_{mapped_size} {}
        void operator() (std::uint8_t * p) const { munmap (p, mapped_size_); }

    private:
        std::size_t mapped_size_;
    };


    std::unique_ptr<std::uint8_t, deleter> memory_map (int fd, std::size_t mapped_size) {
        auto mptr =
            static_cast<std::uint8_t *> (mmap (nullptr, mapped_size, PROT_READ | PROT_WRITE,
                                               MAP_FILE | MAP_SHARED, fd, off_t{0} /*offset*/));
        if (mptr == MAP_FAILED) {
            throw std::system_error{errno, std::generic_category ()};
        }
        return {mptr, deleter{mapped_size}};
    }


    bool file_is_available (char const * path) {
        struct stat stat_buf;
        if (stat (path, &stat_buf) == 0) {
            return true;
        }
        if (errno != ENOENT) {
            throw std::system_error{errno, std::generic_category ()};
        }
        return false; // file wasn't found: that's fine.
    }



    blocks_type load_blocks (std::istream & is, std::uint8_t * base) {
        blocks_type map;
        auto size = read<std::size_t> (is);
        using value_type = blocks_type::value_type::second_type;
        for (; size > 0; --size) {
            auto const v1 = read<std::ptrdiff_t> (is) + base;
            auto const v2 = read<std::tuple_element<0, value_type>::type> (is);
            auto const v3 = read<std::tuple_element<1, value_type>::type> (is);
            map[v1] = std::make_pair (v2, v3);
        }
        return map;
    }

    void save_blocks (std::ostream & os, blocks_type const & blocks, std::uint8_t * base) {
        write (os, blocks.size ());
        for (auto const & v : blocks) {
            write (os, v.first - base);
            write (os, std::get<0> (v.second));
            write (os, std::get<1> (v.second));
        }
    }

    void mmap_stress () {
        constexpr auto alloc_persist = "./map.alloc";
        constexpr auto store_persist = "./store.alloc";
        constexpr auto blocks_persist = "./blocks.alloc";

        constexpr auto mapped_size = std::size_t{1024} * std::size_t{1024};
        constexpr auto num_passes = 16U;
        constexpr auto max_allocation_size = std::size_t{256};
        constexpr auto num_allocations = mapped_size / max_allocation_size;



        int fd = open (store_persist, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
        if (fd == -1) {
            throw std::system_error{errno, std::generic_category ()};
        }
        if (ftruncate (fd, mapped_size) != 0) {
            throw std::system_error{errno, std::generic_category ()};
        }

        auto backing_ptr = memory_map (fd, mapped_size);
        allocator alloc{[](std::size_t /*size*/) {
                            return std::pair<std::uint8_t *, std::size_t> (nullptr, 0);
                        },
                        std::make_pair (backing_ptr.get (), mapped_size)};

        if (file_is_available (alloc_persist)) {
            std::ifstream file (alloc_persist, std::ios::binary);
            alloc.load (file, backing_ptr.get ());
        }

        blocks_type blocks;

        if (file_is_available (blocks_persist)) {
            std::ifstream file (blocks_persist, std::ios::binary);
            blocks = load_blocks (file, backing_ptr.get ());
            if (!blocks_okay (blocks, alloc)) {
                throw bad_memory ();
            }
        }


        std::cout << "On start: " << alloc.allocated_space () << " allocated bytes ("
                  << alloc.num_allocs () << " allocations), " << alloc.free_space ()
                  << " free bytes (" << alloc.num_frees () << " blocks).\n";

        std::mt19937 random;

        allocate_test (num_passes, num_allocations, max_allocation_size, random, blocks, &alloc);
        realloc_test (num_passes, num_allocations, max_allocation_size, random, blocks, &alloc);

        // Free some of our allocations.
        if (alloc.num_allocs () > 0) {
            for (auto ctr = random () % alloc.num_allocs (); ctr > 0U; --ctr) {
                auto pos = alloc.allocs_begin ();
                std::advance (pos, random () % alloc.num_allocs ());
                auto const addr = pos->first;
                alloc.free (addr);
                blocks.erase (addr);
            }

            if (!blocks_okay (blocks, alloc)) {
                throw bad_memory ();
            }
        }

        {
            std::ofstream allocs_file (alloc_persist, std::ios::binary | std::ios::trunc);
            alloc.save (allocs_file, backing_ptr.get ());
        }
        {
            std::ofstream file (blocks_persist, std::ios::binary | std::ios::trunc);
            save_blocks (file, blocks, backing_ptr.get ());
        }
    }

} // end anonymous namespace

int main () {
    int exit_code = EXIT_SUCCESS;
    try {
        mmap_stress ();
    } catch (std::exception const & ex) {
        std::cerr << "Error: " << ex.what () << '\n';
        exit_code = EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown error\n";
        exit_code = EXIT_FAILURE;
    }
    return exit_code;
}

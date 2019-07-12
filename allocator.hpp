#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include <cstdint>
#include <cstdlib>
#include <iosfwd>
#include <map>
#include <utility>

class allocator {
public:
    using address = std::uint8_t *;

    allocator ();
    allocator (allocator const & ) = delete;
    allocator (allocator && ) = delete;

    ~allocator ();

    allocator & operator= (allocator const & ) = delete;
    allocator & operator= (allocator && ) = delete;

    [[nodiscard]] address allocate (std::size_t size);
    void free (address offset);

    void dump (std::ostream & os);
    [[nodiscard]] std::size_t num_allocs () const noexcept { return allocs_.size (); }
    [[nodiscard]] std::size_t num_frees () const noexcept { return frees_.size (); }

private:
    address max_;
    std::map<address, std::size_t> allocs_;
    std::map<address, std::size_t> frees_;
};

#endif // ALLOCATOR_HPP

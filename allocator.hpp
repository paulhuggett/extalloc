#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include <cstdint>
#include <cstdlib>
#include <iosfwd>
#include <map>

class allocator {
public:
    using address = std::uint8_t *;

    [[nodiscard]] address allocate (std::size_t size);
    void free (address offset);

    void dump (std::ostream & os);
    [[nodiscard]] std::size_t num_allocs () const noexcept { return allocs_.size (); }
    [[nodiscard]] std::size_t num_frees () const noexcept { return frees_.size (); }

private:
    address max_ = nullptr;
    // A ordered key/value container.
    using container = std::map<address, std::size_t>;
    container allocs_;
    container frees_;
};

#endif // ALLOCATOR_HPP

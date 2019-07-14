#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iosfwd>
#include <map>

class allocator {
public:
    using address = std::uint8_t *;

    using address_size_pair = std::pair<address, std::size_t>;
    using add_storage_fn = std::function<address_size_pair (std::size_t)>;
    explicit allocator (add_storage_fn as);

    [[nodiscard]] address allocate (std::size_t size);
    void free (address offset);

    void dump (std::ostream & os);
    [[nodiscard]] std::size_t num_allocs () const noexcept { return allocs_.size (); }
    [[nodiscard]] std::size_t num_frees () const noexcept { return frees_.size (); }

private:
    add_storage_fn add_storage_;

    // A ordered key/value container.
    using container = std::map<address, std::size_t>;
    container allocs_;
    container frees_;
};

#endif // ALLOCATOR_HPP

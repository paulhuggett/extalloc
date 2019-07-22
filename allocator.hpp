#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iosfwd>
#include <map>

class no_allocation : public std::runtime_error {
public:
    no_allocation ();
    ~no_allocation () noexcept override;
};


class allocator {
public:
    using address = std::uint8_t *;

    using address_size_pair = std::pair<address, std::size_t>;
    using add_storage_fn = std::function<address_size_pair (std::size_t)>;
    explicit allocator (add_storage_fn as);

    [[nodiscard]] address allocate (std::size_t size);
    void free (address offset);
    [[nodiscard]] address realloc (address ptr, std::size_t new_size);

    bool check () const;
    void dump (std::ostream & os);
    [[nodiscard]] std::size_t num_allocs () const noexcept { return allocs_.size (); }
    [[nodiscard]] std::size_t num_frees () const noexcept { return frees_.size (); }

private:
    add_storage_fn add_storage_;

    // An ordered key/value container.
    using container = std::map<address, std::size_t>;

    static address allocation_end (container::value_type const & p) noexcept {
        return p.first + p.second;
    }


    container allocs_;
    container frees_;
};

#endif // ALLOCATOR_HPP

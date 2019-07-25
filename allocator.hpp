#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <istream>
#include <ostream>
#include <map>
#include <stdexcept>

template <typename T, typename = typename std::enable_if<std::is_standard_layout<T>::value>::type>
inline void write (std::ostream & os, T const & t) {
    os.write (reinterpret_cast<std::ostream::char_type const *> (&t), sizeof (t));
}

template <typename T, typename = typename std::enable_if<std::is_standard_layout<T>::value>::type>
inline T read (std::istream & is) {
    T t;
    is.read (reinterpret_cast<std::istream::char_type *> (&t), sizeof (t));
    return t;
}



class no_allocation : public std::runtime_error {
public:
    no_allocation ();
    ~no_allocation () noexcept override;
};


class allocator {
public:
    using address = std::uint8_t *;
    using container = std::map<address, std::size_t>;

    using address_size_pair = std::pair<address, std::size_t>;
    using add_storage_fn = std::function<address_size_pair (std::size_t)>;

    explicit allocator (address initial_ptr, std::size_t initial_size, add_storage_fn as);

    [[nodiscard]] address allocate (std::size_t size);
    void free (address offset);
    [[nodiscard]] address realloc (address ptr, std::size_t new_size);

    bool check () const;
    void dump (std::ostream & os);

    [[nodiscard]] std::size_t num_allocs () const noexcept { return allocs_.size (); }
    [[nodiscard]] std::size_t num_frees () const noexcept { return frees_.size (); }
    [[nodiscard]] std::size_t allocated_space () const noexcept;
    [[nodiscard]] std::size_t free_space () const noexcept;

    container::const_iterator allocs_begin () { return allocs_.begin (); }
    container::const_iterator allocs_end () { return allocs_.end (); }
    container::const_iterator frees_begin () { return frees_.begin (); }
    container::const_iterator freed_end () { return frees_.end (); }

    std::ostream & save (std::ostream & os, std::uint8_t const * base = nullptr);
    void load (std::istream & is, std::uint8_t * base = nullptr);

    using memory_map = std::map<address, std::tuple<std::size_t, bool>>;

private:
    add_storage_fn add_storage_;

    // An ordered key/value container.

    static address allocation_end (container::value_type const & p) noexcept {
        return p.first + p.second;
    }

    template <typename Container>
    static std::size_t accumulate_values (Container const & c);

    container allocs_;
    container frees_;
};

#endif // ALLOCATOR_HPP

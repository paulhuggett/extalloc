#ifndef EXTALLOC_ALLOCATOR_HPP
#define EXTALLOC_ALLOCATOR_HPP

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <istream>
#include <ostream>
#include <map>
#include <stdexcept>
#include <utility>

namespace extalloc {

    template <typename T,
              typename = typename std::enable_if<std::is_standard_layout<T>::value>::type>
    inline void write (std::ostream & os, T const & t) {
        os.write (reinterpret_cast<std::ostream::char_type const *> (&t), sizeof (t));
    }

    template <typename T,
              typename = typename std::enable_if<std::is_standard_layout<T>::value>::type>
    inline T read (std::istream & is) {
        T t;
        is.read (reinterpret_cast<std::istream::char_type *> (&t), sizeof (t));
        return t;
    }



    class no_allocation : public std::runtime_error {
    public:
        no_allocation ();
        no_allocation (no_allocation const &) = default;
        no_allocation (no_allocation &&) noexcept = default;

        ~no_allocation () noexcept override;

        no_allocation & operator= (no_allocation const &) = default;
        no_allocation & operator= (no_allocation &&) noexcept = default;
    };


    class allocator {
    public:
        using address = std::uint8_t *;
        /// The choice of std::map<> is not significant: the code assumes an ordered key/value
        /// container.
        using container = std::map<address, std::size_t>;

        using add_storage_fn = std::function<std::pair<address, std::size_t> (std::size_t)>;

        /// \param as  A function with signature compatible with `std::pair<address,
        /// size_t>(std::size_t)` which will be called if an allocation request cannot be satisfied.
        /// It is passed the amount of storage requested and should respond by attempting to
        /// allocate at least much. On success it should return a pair containing the base pointer
        /// and the actual allocated size.
        explicit allocator (add_storage_fn const & as)
                : allocator (as, std::make_pair (nullptr, 0)) {}

        /// \param init  The address and size of an initial storage allocation.
        /// \param as  A function with signature compatible with `std::pair<address,
        /// size_t>(std::size_t)` which will be called if an allocation request cannot be satisfied.
        /// It is passed the amount of storage requested and should respond by attempting to
        /// allocate at least much. On success it should return a pair containing the base pointer
        /// and the actual allocated size.
        allocator (add_storage_fn const & as, std::pair<address, std::size_t> const & init);

        address allocate (std::size_t size);
        void free (address offset);
        address realloc (address ptr, std::size_t new_size);

        bool check () const;
        void dump (std::ostream & os);

        std::size_t num_allocs () const noexcept { return allocs_.size (); }
        std::size_t num_frees () const noexcept { return frees_.size (); }
        std::size_t allocated_space () const noexcept;
        std::size_t free_space () const noexcept;

        container::const_iterator allocs_begin () { return allocs_.begin (); }
        container::const_iterator allocs_end () { return allocs_.end (); }
        container::const_iterator frees_begin () { return frees_.begin (); }
        container::const_iterator freed_end () { return frees_.end (); }

        std::ostream & save (std::ostream & os, std::uint8_t const * base = nullptr) const;
        void load (std::istream & is, std::uint8_t * base = nullptr);


    private:
        add_storage_fn add_storage_;

        static address allocation_end (container::value_type const & p) noexcept {
            return p.first + p.second;
        }

        template <typename Container>
        static std::size_t accumulate_values (Container const & c);

        container allocs_;
        container frees_;
    };

} // end namespace extalloc

#endif // EXTALLOC_ALLOCATOR_HPP

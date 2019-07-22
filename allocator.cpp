#include "allocator.hpp"

#include <algorithm>
#include <cassert>
#include <optional>
#include <ostream>
#include <stdexcept>


//*                  _ _              _   _           *
//*  _ _  ___   __ _| | |___  __ __ _| |_(_)___ _ _   *
//* | ' \/ _ \ / _` | | / _ \/ _/ _` |  _| / _ \ ' \  *
//* |_||_\___/ \__,_|_|_\___/\__\__,_|\__|_\___/_||_| *
//*                                                   *
no_allocation::no_allocation ()
        : std::runtime_error ("no allocation") {}
no_allocation::~no_allocation () noexcept = default;

//*       _ _              _            *
//*  __ _| | |___  __ __ _| |_ ___ _ _  *
//* / _` | | / _ \/ _/ _` |  _/ _ \ '_| *
//* \__,_|_|_\___/\__\__,_|\__\___/_|   *
//*                                     *
// ctor
// ~~~~
allocator::allocator (add_storage_fn as)
        : add_storage_{as} {}

// allocate
// ~~~~~~~~
auto allocator::allocate (std::size_t size) -> address {
    address result{};
    size = std::max (size, std::size_t{1});

    if (frees_.empty ()) {
        // No free space at all: allocate more.
        frees_.insert (add_storage_ (size));
    }

    auto const end = std::end (frees_);
    auto pos = std::find_if (std::begin (frees_), end, [size](container::value_type const & vt) {
        return vt.second >= size;
    });
    if (pos == end) {
        // No free space large enough: allocate more.
        bool inserted = false;
        std::tie (pos, inserted) = frees_.insert (add_storage_ (size));
    }

    // There's a free block with sufficient space.
    result = pos->first;

    // Split this block?
    assert (pos->second >= size);
    if (pos->second > size) {
        frees_.insert ({result + size, pos->second - size});
    }
    frees_.erase (pos);

    allocs_.insert ({result, size});
    return result;
}

// realloc
// ~~~~~~~
[[nodiscard]] auto allocator::realloc (address ptr, std::size_t new_size) -> address {
    new_size = std::max (new_size, std::size_t{1});

    auto const pos = allocs_.find (ptr);
    assert (frees_.find (ptr) == std::end (frees_));
    if (pos == std::end (allocs_)) {
        throw no_allocation ();
    }

    if (new_size == pos->second) {
        // no change in size: just return the original pointer.
        return ptr;
    }

    auto const end_address = allocation_end (*pos);
    auto const lb = frees_.lower_bound (end_address);

    if (new_size > pos->second) {
        // We're being asked to enlarge the allocation. Is there sufficient free space immediately
        // following?
        auto const extra = new_size - pos->second;
        if (lb != std::end (frees_) && lb->first == end_address && lb->second >= extra) {
            auto const f = *lb;
            frees_.erase (lb);
            if (f.second > extra) {
                frees_.insert ({end_address + extra, f.second - extra});
            }
            pos->second = new_size;
            return ptr;
        }

        // We must move the block somewhere else to satisfy the allocation request/
        auto new_ptr = this->allocate (new_size);
        std::copy (ptr, ptr + pos->second, new_ptr);
        this->free (pos->first);
        return new_ptr;
    }

    assert (new_size < pos->second);
    auto const reduction = pos->second - new_size;
    if (lb != std::end (frees_) && lb->first == end_address) {
        // There's a free block immediately following. Move its start to coincide with the space
        // being released.
        auto const f = std::make_pair (lb->first - reduction, lb->second + reduction);
        assert (allocation_end (f) == allocation_end (*lb));
        frees_.erase (lb);
        frees_.insert (f);

        // Adjust the allocation size.
        pos->second = new_size;
        return ptr;
    }

    frees_.insert ({ptr + new_size, reduction});
    pos->second = new_size;
    return ptr;
}

// free
// ~~~~
void allocator::free (address offset) {
    auto const pos = allocs_.find (offset);
    assert (frees_.find (offset) == std::end (frees_));
    if (pos == std::end (allocs_)) {
        throw no_allocation ();
    }

    std::optional<container::iterator> prev;
    std::optional<container::iterator> next;

    // lower_bound() returns an iterator pointing to the first element that's not less than offset.
    auto lb = frees_.lower_bound (offset);
    if (lb != std::begin (frees_)) {
        prev = lb;
        std::advance (*prev, -1);
        if (offset != allocation_end (**prev)) {
            prev.reset ();
        }
    }
    if (lb != std::end (frees_)) {
        assert (lb->first > offset);
        if (allocation_end (*pos) == lb->first) {
            next = lb;
        }
    }

    if (prev) {
        if (next) {
            // We can merge with both the previous and subsequent free. This merges the 3 frees
            // into a single record.
            (*prev)->second += pos->second + (*next)->second;
            frees_.erase (*next);
        } else {
            // We can merge with the previous free. No new record is necessary.
            (*prev)->second += pos->second;
        }
    } else if (next) {
        // We can merge with the subsequent free. We create a record for this concatenated region
        // and release the original.
        frees_.insert ({pos->first, pos->second + (*next)->second});
        frees_.erase (*next);
    } else {
        // We can't merge: create a new record.
        frees_.insert (*pos);
    }

    allocs_.erase (pos);
}

// dump
// ~~~~
void allocator::dump (std::ostream & os) {
    auto const emit = [&os](std::pair<address, std::size_t> const & v) {
        auto const first = reinterpret_cast<std::uintptr_t> (v.first);
        os << "  [" << first << ',' << first + v.second << ")\n";
    };
    os << "Allocations:\n";
    std::for_each (std::begin (allocs_), std::end (allocs_), emit);
    os << "Frees:\n";
    std::for_each (std::begin (frees_), std::end (frees_), emit);
}

// check
// ~~~~~
bool allocator::check () const {
    container map = allocs_;
    for (auto const & m : frees_) {
        if (map.find (m.first) != map.end ()) {
            return false;
        }
        map[m.first] = m.second;
    }

    if (!map.empty ()) {
        auto it = map.begin ();
        auto end = map.end ();
        auto addr = it->first + it->second;
        ++it;
        for (; it != end; ++it) {
            if (it->first < addr) {
                return false;
            }
            addr = it->first + it->second;
        }
    }
    return true;
}

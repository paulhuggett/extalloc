#include "allocator.hpp"

#include <algorithm>
#include <cassert>
#include <optional>
#include <ostream>
#include <stdexcept>

// allocate
// ~~~~~~~~
auto allocator::allocate (std::size_t size) -> address {
    address result{};
    size = std::max (size, std::size_t{1});

    if (frees_.empty ()) {
        // No free space at all: allocate more.
        result = max_;
        max_ += size;
    } else {
        auto const end = std::end (frees_);
        auto const pos = std::find_if (
            std::begin (frees_), end,
            [size](container::value_type const & vt) { return vt.second >= size; });
        if (pos == end) {
            // No free space large enough: allocate more.
            result = max_;
            max_ += size;
        } else {
            // There's a free block with sufficient space.
            result = pos->first;
            assert (pos->second >= size);
            auto new_size = pos->second - size;
            if (pos->second > size) {
                frees_.insert ({result + size, new_size});
            }
            frees_.erase (pos);
        }
    }
    allocs_.insert ({result, size});
    return result;
}

// free
// ~~~~
void allocator::free (address offset) {
    auto const pos = allocs_.find (offset);
    assert (frees_.find (offset) == std::end (frees_));
    if (pos == std::end (allocs_)) {
        throw std::runtime_error ("no allocation");
    }

    constexpr auto allocation_end = [](container::value_type const & p) noexcept {
        return p.first + p.second;
    };

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

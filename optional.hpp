#ifndef EXTALLOC_OPTIONAL_HPP
#define EXTALLOC_OPTIONAL_HPP

#include <cassert>
#include <new>
#include <stdexcept>
#include <type_traits>

namespace details {

    template <typename T>
    struct remove_cvref {
        using type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
    };
    template <typename T>
    using remove_cvref_t = typename remove_cvref<T>::type;

} // end namespace details

template <typename T, typename = typename std::enable_if<!std::is_reference<T>::value>::type>
class optional {
public:
    using value_type = T;

    static_assert (std::is_object<value_type>::value,
                   "Instantiation of optional<> with a non-object type is undefined behavior.");
    static_assert (std::is_nothrow_destructible<value_type>::value,
                   "Instantiation of optional<> with an object type that is not noexcept "
                   "destructible is undefined behavior.");

    /// Constructs an object that does not contain a value.
    constexpr optional () noexcept = default;

    /// Constructs an optional object that contains a value, initialized with the expression
    /// std::forward<U>(value).
    template <typename U,
              typename = typename std::enable_if<
                  std::is_constructible<T, U &&>::value &&
                  !std::is_same<typename details::remove_cvref_t<U>, optional<T>>::value>::type>
    explicit optional (U && value) noexcept (std::is_nothrow_move_constructible<T>::value &&
                                                 std::is_nothrow_copy_constructible<T>::value &&
                                             !std::is_convertible<U &&, T>::value) {

        new (&storage_) T (std::forward<U> (value));
        valid_ = true;
    }

    /// Copy constructor: If other contains a value, initializes the contained value with the
    /// expression *other. If other does not contain a value, constructs an object that does not
    /// contain a value.
    optional (optional const & other) noexcept (std::is_nothrow_copy_constructible<T>::value) {
        if (other.valid_) {
            new (&storage_) T (*other);
            valid_ = true;
        }
    }

    /// Move constructor: If other contains a value, initializes the contained value with the
    /// expression std::move(*other) and does not make other empty: a moved-from optional still
    /// contains a value, but the value itself is moved from. If other does not contain a value,
    /// constructs an object that does not contain a value.
    optional (optional && other) noexcept (std::is_nothrow_move_constructible<T>::value) {
        if (other.valid_) {
            new (&storage_) T (std::move (*other));
            valid_ = true;
        }
    }

    ~optional () noexcept { this->reset (); }

    /// If *this contains a value, destroy it. *this does not contain a value after this call.
    void reset () noexcept {
        if (valid_) {
            // Set valid_ to false before calling the dtor.
            valid_ = false;
            reinterpret_cast<T const *> (&storage_)->~T ();
        }
    }

    optional & operator= (optional const & other) noexcept (
        std::is_nothrow_copy_assignable<T>::value && std::is_nothrow_copy_constructible<T>::value) {
        if (&other != this) {
            if (!other.has_value ()) {
                this->reset ();
            } else {
                this->operator= (other.value ());
            }
        }
        return *this;
    }

    optional & operator= (optional && other) noexcept (
        std::is_nothrow_move_assignable<T>::value && std::is_nothrow_move_constructible<T>::value) {

        if (&other != this) {
            if (!other.has_value ()) {
                this->reset ();
            } else {
                this->operator= (std::forward<T> (other.value ()));
            }
        }
        return *this;
    }

    template <typename U,
              typename = typename std::enable_if<
                  std::is_constructible<T, U &&>::value &&
                  !std::is_same<typename details::remove_cvref_t<U>, optional<T>>::value>::type>
    optional & operator= (U && other) noexcept (
        std::is_nothrow_copy_assignable<T>::value && std::is_nothrow_copy_constructible<T>::value &&
            std::is_nothrow_move_assignable<T>::value &&
                std::is_nothrow_move_constructible<T>::value) {

        if (valid_) {
            T temp = std::forward<U> (other);
            std::swap (this->value (), temp);
        } else {
            new (&storage_) T (std::forward<U> (other));
            valid_ = true;
        }
        return *this;
    }

    bool operator== (optional const & other) const {
        return this->has_value () == other.has_value () &&
               (!this->has_value () || this->value () == other.value ());
    }
    bool operator!= (optional const & other) const { return !operator== (other); }

    /// accesses the contained value
    T const & operator* () const noexcept { return *(operator-> ()); }
    /// accesses the contained value
    T & operator* () noexcept { return *(operator-> ()); }
    /// accesses the contained value
    T const * operator-> () const noexcept { return &value (); }
    /// accesses the contained value
    T * operator-> () noexcept { return &value (); }

    /// checks whether the object contains a value
    constexpr explicit operator bool () const noexcept { return valid_; }
    /// checks whether the object contains a value
    constexpr bool has_value () const noexcept { return valid_; }

    T const & value () const noexcept {
        assert (valid_);
        return reinterpret_cast<T const &> (storage_);
    }
    T & value () noexcept {
        assert (valid_);
        return reinterpret_cast<T &> (storage_);
    }

    template <typename U>
    constexpr T value_or (U && default_value) const {
        return this->has_value () ? this->value () : default_value;
    }

private:
    bool valid_ = false;
    typename std::aligned_storage<sizeof (T), alignof (T)>::type storage_;
};

#endif // EXTALLOC_OPTIONAL_HPP

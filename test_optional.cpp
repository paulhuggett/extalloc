#include "optional.hpp"

#include <algorithm>

#include <gtest/gtest.h>

using namespace extalloc;

namespace {

    class value {
    public:
        explicit value (int v)
                : v_{new int(v)} {}
        ~value () noexcept = default;

        value (value const & other);
        value (value && other) noexcept = default;
        value & operator= (value const & other);
        value & operator= (value && other) noexcept = default;

        bool operator== (value const & rhs) const { return this->get () == rhs.get (); }

        int get () const { return *v_; }

    private:
        std::unique_ptr<int> v_;
    };

    value::value (value const & other)
            : v_{new int (other.get ())} {}

    value & value::operator= (value const & other) {
        if (this != &other) {
            v_.reset (new int (other.get ()));
        }
        return *this;
    }

} // end anonymous namespace

TEST (Optional, NoValue) {
    optional<value> m;
    EXPECT_FALSE (m.operator bool ());
    EXPECT_FALSE (m.has_value ());
    EXPECT_FALSE (m);
}

TEST (Optional, Value) {
    optional<value> m (42);
    EXPECT_TRUE (m.has_value ());
    EXPECT_TRUE (m);
    EXPECT_EQ (m.value (), value (42));

    m.reset ();
    EXPECT_FALSE (m.has_value ());
}

TEST (Optional, CtorWithOptionalHoldingAValue) {
    optional<value> m1 (42);
    optional<value> m2 = m1;
    EXPECT_TRUE (m2.has_value ());
    EXPECT_TRUE (m2);
    EXPECT_EQ (m2.value (), value (42));
}

TEST (Optional, ValueOr) {
    optional<value> m1;
    EXPECT_EQ (m1.value_or (value{37}), value{37});

    optional<value> m2 (5);
    EXPECT_EQ (m2.value_or (value{37}), value{5});

    optional<value> m3 (7);
    EXPECT_EQ (m2.value_or (value{37}), value{5});
}

TEST (Optional, AssignValue) {
    optional<value> m;

    // First assignment, m has no value
    m.operator=(value{43});
    EXPECT_TRUE (m.has_value ());
    EXPECT_TRUE (m);
    EXPECT_EQ (m.value (), value{43});

    // Second assignment, m holds a value
    m.operator=(value{44});
    EXPECT_TRUE (m.has_value ());
    EXPECT_TRUE (m);
    EXPECT_EQ (m.value (), value{44});

    // Third assignment, m holds a value, assigning nothing.
    m.operator= (optional<value> ());
    EXPECT_FALSE (m.has_value ());
}

TEST (Optional, AssignZero) {
    optional<char> m;
    m = char{0};
    EXPECT_TRUE (m.operator bool ());
    EXPECT_TRUE (m.has_value ());
    EXPECT_EQ (m.value (), 0);
    EXPECT_EQ (*m, 0);

    optional<char> m2 = m;
    EXPECT_TRUE (m2.has_value ());
    EXPECT_EQ (m2.value (), 0);
    EXPECT_EQ (*m2, 0);
}

TEST (Optional, MoveCtor) {
    {
        optional<std::string> m1{std::string{"test"}};
        EXPECT_TRUE (m1.has_value ());
        EXPECT_EQ (m1.value (), "test");

        optional<std::string> m2 = std::move (m1);
        EXPECT_TRUE (m2.has_value ());
        EXPECT_EQ (*m2, "test");
    }
    {
        // Now move to a optional<> with no initial value.
        optional<std::string> m3;
        optional<std::string> m4 = std::move (m3);
        EXPECT_FALSE (m4.has_value ());
    }
}

TEST (Optional, MoveAssign) {
    // No initial value
    {
        optional<std::string> m1;
        EXPECT_FALSE (m1);
        m1 = std::string{"test"};
        EXPECT_TRUE (m1.has_value ());
        EXPECT_EQ (m1.value (), std::string{"test"});
    }
    // With an initial value
    {
        optional<std::string> m2{"before"};
        EXPECT_TRUE (m2);
        optional<std::string> m3{"after"};
        EXPECT_TRUE (m2);

        m2 = std::move (m3);
        EXPECT_TRUE (m2.has_value ());
        EXPECT_TRUE (m3.has_value ()); // a moved-from optional still contains a value.
        EXPECT_EQ (m2.value (), std::string{"after"});
    }
}

TEST (Optional, CopyAssign) {
    // both lhs and rhs have no value.
    {
        optional<std::string> m1;
        optional<std::string> const m2{};
        m1.operator= (m2);
        EXPECT_FALSE (m1.has_value ());
    }

    // lhs with no value, rhs with value.
    {
        optional<std::string> m1;
        optional<std::string> const m2 ("test");
        m1.operator= (m2);
        ASSERT_TRUE (m1.has_value ());
        EXPECT_EQ (m1.value (), std::string{"test"});
    }

    // lhs with value, rhs with no value.
    {
        optional<std::string> m1 ("test");
        optional<std::string> m2;
        m1.operator= (m2);
        EXPECT_FALSE (m1.has_value ());
    }

    // both lhs and rhs have a value.
    {
        optional<std::string> m1 ("original");
        optional<std::string> m2 ("new");
        m1.operator= (m2);
        ASSERT_TRUE (m1.has_value ());
        EXPECT_EQ (m1.value (), std::string{"new"});
    }
}

TEST (Optional, SelfAssign) {
    // self-assign with no value
    {
        optional<std::string> m1;
        m1.operator= (m1);
        EXPECT_FALSE (m1.has_value ());
    }

    // self-assign with a value
    {
        optional<std::string> m1 ("test");
        m1.operator= (m1);
        ASSERT_TRUE (m1.has_value ());
        EXPECT_EQ (m1.value (), std::string{"test"});
    }
}

TEST (Optional, Equal) {
    {
        // neither lhs nor rhs have a value
        optional<int> m1;
        optional<int> m2;
        EXPECT_TRUE (m1 == m2);
        EXPECT_FALSE (m1 != m2);
    }
    {
        // lhs has a value, rhs not.
        optional<int> m1 (3);
        optional<int> m2;
        EXPECT_FALSE (m1 == m2);
        EXPECT_TRUE (m1 != m2);
    }

    {
        // lhs has no value, rhs does.
        optional<int> m1;
        optional<int> m2 (5);
        EXPECT_FALSE (m1 == m2);
        EXPECT_TRUE (m1 != m2);
    }
    {
        // both lhs and rhs have values but they are different.
        optional<int> m1 (7);
        optional<int> m2 (11);
        EXPECT_FALSE (m1 == m2);
        EXPECT_TRUE (m1 != m2);
    }
    {
        // lhs both have the same value.
        optional<int> m1 (13);
        optional<int> m2 (13);
        EXPECT_TRUE (m1 == m2);
        EXPECT_FALSE (m1 != m2);
    }
}

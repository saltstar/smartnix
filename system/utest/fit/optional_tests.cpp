// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <type_traits>

#include <lib/fit/optional.h>
#include <unittest/unittest.h>

#include "unittest_utils.h"

namespace {

template <bool define_assignment_operators>
struct base {};
template <>
struct base<false> {
    base& operator=(const base& other) = delete;
    base& operator=(base&& other) = delete;
};

template <bool define_assignment_operators>
struct slot : base<define_assignment_operators> {
    slot(int value = 0)
        : value(value) {
        balance++;
    }
    slot(const slot& other)
        : value(other.value) {
        balance++;
    }
    slot(slot&& other)
        : value(other.value) {
        balance++;
    }

    ~slot() {
        ASSERT_CRITICAL(balance > 0);
        ASSERT_CRITICAL(value != -1);
        value = -1; // sentinel to catch double-delete
        balance--;
    }

    static int balance; // net constructor/destructor pairings
    int value;

    int get() const { return value; }
    int increment() { return ++value; }

    slot& operator=(const slot& other) = default;
    slot& operator=(slot&& other) = default;

    bool operator==(const slot& other) const { return value == other.value; }
    bool operator!=(const slot& other) const { return value != other.value; }
};
template <>
int slot<false>::balance = 0;
template <>
int slot<true>::balance = 0;

// Test optional::value_type.
static_assert(std::is_same<int, fit::optional<int>::value_type>::value, "");

// TODO(US-90): Unfortunately fit::optional is not a literal type unlike
// std::optional so expressions involving fit::optional are not constexpr.
// Once we fix that, we should add static asserts to check cases where
// fit::optional wraps a literal type.

template <typename T>
bool construct_without_value() {
    BEGIN_TEST;

    fit::optional<T> opt;
    EXPECT_FALSE(opt.has_value());
    EXPECT_FALSE(!!opt);

    EXPECT_EQ(42, opt.value_or(T{42}).value);

    opt.reset();
    EXPECT_FALSE(opt.has_value());

    END_TEST;
}

template <typename T>
bool construct_with_value() {
    BEGIN_TEST;

    fit::optional<T> opt(T{42});
    EXPECT_TRUE(opt.has_value());
    EXPECT_TRUE(!!opt);

    EXPECT_EQ(42, opt.value().value);
    EXPECT_EQ(42, opt.value_or(T{55}).value);

    EXPECT_EQ(42, opt->get());
    EXPECT_EQ(43, opt->increment());
    EXPECT_EQ(43, opt->get());

    opt.reset();
    EXPECT_FALSE(opt.has_value());

    END_TEST;
}

template <typename T>
bool construct_copy() {
    BEGIN_TEST;

    fit::optional<T> a(T{42});
    fit::optional<T> b(a);
    fit::optional<T> c;
    fit::optional<T> d(c);
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(42, a.value().value);
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(42, b.value().value);
    EXPECT_FALSE(c.has_value());
    EXPECT_FALSE(d.has_value());

    END_TEST;
}

template <typename T>
bool construct_move() {
    BEGIN_TEST;

    fit::optional<T> a(T{42});
    fit::optional<T> b(std::move(a));
    fit::optional<T> c;
    fit::optional<T> d(std::move(c));
    EXPECT_FALSE(a.has_value());
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(42, b.value().value);
    EXPECT_FALSE(c.has_value());
    EXPECT_FALSE(d.has_value());

    END_TEST;
}

template <typename T>
T get_value(fit::optional<T> opt) {
    return opt.value();
}

bool construct_with_implicit_conversion() {
    BEGIN_TEST;

    // get_value expects a value of type fit::optional<T> but we pass 3
    // so this exercises the converting constructor
    EXPECT_EQ(3, get_value<int>(3));

    END_TEST;
}

template <typename T>
bool accessors() {
    BEGIN_TEST;

    fit::optional<T> a(T{42});
    T& value = a.value();
    EXPECT_EQ(42, value.value);

    const T& const_value = const_cast<const decltype(a)&>(a).value();
    EXPECT_EQ(42, const_value.value);

    T rvalue = fit::optional<T>(T{42}).value();
    EXPECT_EQ(42, rvalue.value);

    T const_rvalue = const_cast<const fit::optional<T>&&>(
                         fit::optional<T>(T{42}))
                         .value();
    EXPECT_EQ(42, const_rvalue.value);

    END_TEST;
}

template <typename T>
bool assign() {
    BEGIN_TEST;

    fit::optional<T> a(T{42});
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(42, a.value().value);

    a = T{99};
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(99, a.value().value);

    a.reset();
    EXPECT_FALSE(a.has_value());

    a = T{55};
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(55, a.value().value);

    a = fit::nullopt;
    EXPECT_FALSE(a.has_value());

    END_TEST;
}

template <typename T>
bool assign_copy() {
    BEGIN_TEST;

    fit::optional<T> a(T{42});
    fit::optional<T> b(T{55});
    fit::optional<T> c;
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(42, a.value().value);
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(55, b.value().value);
    EXPECT_FALSE(c.has_value());

    a = b;
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(55, b.value().value);
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(55, b.value().value);

    b = c;
    EXPECT_FALSE(b.has_value());
    EXPECT_FALSE(c.has_value());

    b = a;
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(55, b.value().value);
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(55, b.value().value);

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif

    b = b;
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(55, b.value().value);

    c = c;
    EXPECT_FALSE(c.has_value());

#ifdef __clang__
#pragma clang diagnostic pop
#endif

    END_TEST;
}

template <typename T>
bool assign_move() {
    BEGIN_TEST;

    fit::optional<T> a(T{42});
    fit::optional<T> b(T{55});
    fit::optional<T> c;
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(42, a.value().value);
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(55, b.value().value);
    EXPECT_FALSE(c.has_value());

    a = std::move(b);
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(55, a.value().value);
    EXPECT_FALSE(b.has_value());

    b = std::move(c);
    EXPECT_FALSE(b.has_value());
    EXPECT_FALSE(c.has_value());

    c = std::move(b);
    EXPECT_FALSE(c.has_value());
    EXPECT_FALSE(b.has_value());

    b = std::move(a);
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(55, b.value().value);
    EXPECT_FALSE(a.has_value());

    b = std::move(b);
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(55, b.value().value);

    a = std::move(a);
    EXPECT_FALSE(a.has_value());

    END_TEST;
}

template <typename T>
bool emplace() {
    BEGIN_TEST;

    fit::optional<T> a;
    EXPECT_EQ(55, a.emplace(55).value);
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(55, a.value().value);

    fit::optional<T> b(T{42});
    EXPECT_EQ(66, b.emplace(66).value);
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(66, b.value().value);

    END_TEST;
}

template <typename T>
bool invoke() {
    BEGIN_TEST;

    fit::optional<T> a(T{42});
    EXPECT_EQ(42, a->get());
    EXPECT_EQ(43, a->increment());
    EXPECT_EQ(43, (*a).value);

    END_TEST;
}

template <typename T>
bool comparisons() {
    BEGIN_TEST;

    fit::optional<T> a(T{42});
    fit::optional<T> b(T{55});
    fit::optional<T> c(T{42});
    fit::optional<T> d;
    fit::optional<T> e;

    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a == c);
    EXPECT_FALSE(a == d);
    EXPECT_TRUE(d == e);
    EXPECT_FALSE(d == a);

    EXPECT_FALSE(a == fit::nullopt);
    EXPECT_FALSE(fit::nullopt == a);
    EXPECT_TRUE(a == T{42});
    EXPECT_TRUE(T{42} == a);
    EXPECT_FALSE(a == T{55});
    EXPECT_FALSE(T{55} == a);
    EXPECT_FALSE(d == T{42});
    EXPECT_FALSE(T{42} == d);
    EXPECT_TRUE(d == fit::nullopt);
    EXPECT_TRUE(fit::nullopt == d);

    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a != c);
    EXPECT_TRUE(a != d);
    EXPECT_FALSE(d != e);
    EXPECT_TRUE(d != a);

    EXPECT_TRUE(a != fit::nullopt);
    EXPECT_TRUE(fit::nullopt != a);
    EXPECT_FALSE(a != T{42});
    EXPECT_FALSE(T{42} != a);
    EXPECT_TRUE(a != T{55});
    EXPECT_TRUE(T{55} != a);
    EXPECT_TRUE(d != T{42});
    EXPECT_TRUE(T{42} != d);
    EXPECT_FALSE(d != fit::nullopt);
    EXPECT_FALSE(fit::nullopt != d);

    END_TEST;
}

template <typename T>
bool swapping() {
    BEGIN_TEST;

    fit::optional<T> a(T{42});
    fit::optional<T> b(T{55});
    fit::optional<T> c;
    fit::optional<T> d;

    swap(a, b);
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(55, a.value().value);
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(42, b.value().value);

    swap(a, c);
    EXPECT_FALSE(a.has_value());
    EXPECT_TRUE(c.has_value());
    EXPECT_EQ(55, c.value().value);

    swap(d, c);
    EXPECT_FALSE(c.has_value());
    EXPECT_TRUE(d.has_value());
    EXPECT_EQ(55, d.value().value);

    swap(c, a);
    EXPECT_FALSE(c.has_value());
    EXPECT_FALSE(a.has_value());

    swap(a, a);
    EXPECT_FALSE(a.has_value());

    swap(d, d);
    EXPECT_TRUE(d.has_value());
    EXPECT_EQ(55, d.value().value);

    END_TEST;
}

template <typename T>
bool balance() {
    BEGIN_TEST;

    EXPECT_EQ(0, T::balance);

    END_TEST;
}

} // namespace

BEGIN_TEST_CASE(optional_tests)
RUN_TEST(construct_with_implicit_conversion)
RUN_TEST(construct_without_value<slot<false>>)
RUN_TEST(construct_without_value<slot<true>>)
RUN_TEST(construct_with_value<slot<false>>)
RUN_TEST(construct_with_value<slot<true>>)
RUN_TEST(construct_copy<slot<false>>)
RUN_TEST(construct_copy<slot<true>>)
RUN_TEST(construct_move<slot<false>>)
RUN_TEST(construct_move<slot<true>>)
RUN_TEST(accessors<slot<false>>)
RUN_TEST(accessors<slot<true>>)
RUN_TEST(assign<slot<false>>)
RUN_TEST(assign<slot<true>>)
RUN_TEST(assign_copy<slot<false>>)
RUN_TEST(assign_copy<slot<true>>)
RUN_TEST(assign_move<slot<false>>)
RUN_TEST(assign_move<slot<true>>)
RUN_TEST(emplace<slot<false>>)
RUN_TEST(emplace<slot<true>>)
RUN_TEST(invoke<slot<false>>)
RUN_TEST(invoke<slot<true>>)
RUN_TEST(comparisons<slot<false>>)
RUN_TEST(comparisons<slot<true>>)
RUN_TEST(swapping<slot<false>>)
RUN_TEST(swapping<slot<true>>)
RUN_TEST(balance<slot<false>>)
RUN_TEST(balance<slot<true>>)
END_TEST_CASE(optional_tests)

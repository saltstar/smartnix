// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <functional>

#include <lib/fit/promise.h>
#include <lib/fit/single_threaded_executor.h>
#include <unittest/unittest.h>

#include "examples/utils.h"
#include "unittest_utils.h"

namespace {

class fake_context : public fit::context {
public:
    fit::executor* executor() const override {
        ASSERT_CRITICAL(false);
    }
    fit::suspended_task suspend_task() override {
        ASSERT_CRITICAL(false);
    }
};

template <typename V = void, typename E = void>
class capture_result_wrapper {
public:
    template <typename Promise>
    decltype(auto) wrap(Promise promise) {
        static_assert(std::is_same<V, typename Promise::value_type>::value, "");
        static_assert(std::is_same<E, typename Promise::error_type>::value, "");
        ASSERT_CRITICAL(promise);
        return promise.then([this](fit::result<V, E>& result) {
            last_result = std::move(result);
        });
    }

    fit::result<V, E> last_result;
};

struct move_only {
    move_only(const move_only&) = delete;
    move_only(move_only&&) = default;
    move_only& operator=(const move_only&) = delete;
    move_only& operator=(move_only&&) = default;
};

// Just a simple test to put the promise through its paces.
// Other tests go into more detail to cover the API surface.
bool basics() {
    BEGIN_TEST;

    for (int i = 0; i < 5; i++) {
        // Make a promise that calculates half the square of a number.
        // Produces an error if the square is odd.
        auto promise =
            fit::make_promise([i] {
                // Pretend that squaring numbers is hard and takes time
                // to finish...
                return utils::sleep_for_a_little_while()
                    .then([i](fit::result<>) {
                        return fit::ok(i * i);
                    });
            }).then([](fit::result<int> square) -> fit::result<int, const char*> {
                if (square.value() % 2 == 0)
                    return fit::ok(square.value() / 2);
                return fit::error("square is odd");
            });

        // Evaluate the promise.
        fit::result<int, const char*> result =
            fit::run_single_threaded(std::move(promise));
        if (i % 2 == 0) {
            EXPECT_TRUE(result.is_ok());
            EXPECT_EQ(i * i / 2, result.value());
        } else {
            EXPECT_TRUE(result.is_error());
            EXPECT_STR_EQ("square is odd", result.error());
        }
    }

    END_TEST;
}

// An empty promise has no continuation.
// We can't do a lot with it but we can check for emptyness.
bool empty_promise() {
    BEGIN_TEST;

    {
        fit::promise<> promise;
        EXPECT_FALSE(promise);
    }

    {
        fit::promise<> promise(nullptr);
        EXPECT_FALSE(promise);
    }

    {
        fit::function<fit::result<>(fit::context&)> f;
        fit::promise<> promise(std::move(f));
        EXPECT_FALSE(promise);
    }

    {
        std::function<fit::result<>(fit::context&)> f;
        fit::promise<> promise(std::move(f));
        EXPECT_FALSE(promise);
    }

    END_TEST;
}

bool invocation() {
    BEGIN_TEST;

    uint64_t run_count = 0;
    fake_context fake_context;
    fit::promise<> promise([&](fit::context& context) -> fit::result<> {
        ASSERT_CRITICAL(&context == &fake_context);
        if (++run_count == 2)
            return fit::ok();
        return fit::pending();
    });
    EXPECT_TRUE(promise);

    fit::result<> result = promise(fake_context);
    EXPECT_EQ(1, run_count);
    EXPECT_EQ(fit::result_state::pending, result.state());
    EXPECT_TRUE(promise);

    result = promise(fake_context);
    EXPECT_EQ(2, run_count);
    EXPECT_EQ(fit::result_state::ok, result.state());
    EXPECT_FALSE(promise);

    END_TEST;
}

bool take_continuation() {
    BEGIN_TEST;

    uint64_t run_count = 0;
    fake_context fake_context;
    fit::promise<> promise([&](fit::context& context) -> fit::result<> {
        ASSERT_CRITICAL(&context == &fake_context);
        run_count++;
        return fit::pending();
    });
    EXPECT_TRUE(promise);

    fit::function<fit::result<>(fit::context&)> f = promise.take_continuation();
    EXPECT_FALSE(promise);
    EXPECT_EQ(0, run_count);

    fit::result<> result = f(fake_context);
    EXPECT_EQ(1, run_count);
    EXPECT_EQ(fit::result_state::pending, result.state());

    END_TEST;
}

bool assignment_and_swap() {
    BEGIN_TEST;

    fake_context fake_context;

    fit::promise<> empty;
    EXPECT_FALSE(empty);

    uint64_t run_count = 0;
    fit::promise<> promise([&](fit::context& context) -> fit::result<> {
        run_count++;
        return fit::pending();
    });
    EXPECT_TRUE(promise);

    fit::promise<> x(std::move(empty));
    EXPECT_FALSE(x);

    fit::promise<> y(std::move(promise));
    EXPECT_TRUE(y);
    y(fake_context);
    EXPECT_EQ(1, run_count);

    x.swap(y);
    EXPECT_TRUE(x);
    EXPECT_FALSE(y);
    x(fake_context);
    EXPECT_EQ(2, run_count);

    x.swap(x);
    EXPECT_TRUE(x);
    x(fake_context);
    EXPECT_EQ(3, run_count);

    y.swap(y);
    EXPECT_FALSE(y);

    x = nullptr;
    EXPECT_FALSE(x);

    y = [&](fit::context& context) -> fit::result<> {
        run_count *= 2;
        return fit::pending();
    };
    EXPECT_TRUE(y);
    y(fake_context);
    EXPECT_EQ(6, run_count);

    x = std::move(y);
    EXPECT_TRUE(x);
    EXPECT_FALSE(y);
    x(fake_context);
    EXPECT_EQ(12, run_count);

    x = std::move(y);
    EXPECT_FALSE(x);

    END_TEST;
}

bool comparison_with_nullptr() {
    BEGIN_TEST;

    {
        fit::promise<> promise;
        EXPECT_TRUE(promise == nullptr);
        EXPECT_TRUE(nullptr == promise);
        EXPECT_FALSE(promise != nullptr);
        EXPECT_FALSE(nullptr != promise);
    }

    {
        fit::promise<> promise([&](fit::context& context) -> fit::result<> {
            return fit::pending();
        });
        EXPECT_FALSE(promise == nullptr);
        EXPECT_FALSE(nullptr == promise);
        EXPECT_TRUE(promise != nullptr);
        EXPECT_TRUE(nullptr != promise);
    }

    END_TEST;
}

bool make_promise() {
    BEGIN_TEST;

    fake_context fake_context;

    // Handler signature: void().
    {
        uint64_t run_count = 0;
        auto p = fit::make_promise([&] {
            run_count++;
        });
        static_assert(std::is_same<void, decltype(p)::value_type>::value, "");
        static_assert(std::is_same<void, decltype(p)::error_type>::value, "");
        fit::result<> result = p(fake_context);
        EXPECT_EQ(1, run_count);
        EXPECT_EQ(fit::result_state::ok, result.state());
        EXPECT_FALSE(p);
    }

    // Handler signature: fit::result<int, char>().
    {
        uint64_t run_count = 0;
        auto p = fit::make_promise([&]() -> fit::result<int, char> {
            run_count++;
            return fit::ok(42);
        });
        static_assert(std::is_same<int, decltype(p)::value_type>::value, "");
        static_assert(std::is_same<char, decltype(p)::error_type>::value, "");
        fit::result<int, char> result = p(fake_context);
        EXPECT_EQ(1, run_count);
        EXPECT_EQ(fit::result_state::ok, result.state());
        EXPECT_EQ(42, result.value());
        EXPECT_FALSE(p);
    }

    // Handler signature: fit::ok<int>().
    {
        uint64_t run_count = 0;
        auto p = fit::make_promise([&] {
            run_count++;
            return fit::ok(42);
        });
        static_assert(std::is_same<int, decltype(p)::value_type>::value, "");
        static_assert(std::is_same<void, decltype(p)::error_type>::value, "");
        fit::result<int, void> result = p(fake_context);
        EXPECT_EQ(1, run_count);
        EXPECT_EQ(fit::result_state::ok, result.state());
        EXPECT_EQ(42, result.value());
        EXPECT_FALSE(p);
    }

    // Handler signature: fit::error<int>().
    {
        uint64_t run_count = 0;
        auto p = fit::make_promise([&] {
            run_count++;
            return fit::error(42);
        });
        static_assert(std::is_same<void, decltype(p)::value_type>::value, "");
        static_assert(std::is_same<int, decltype(p)::error_type>::value, "");
        fit::result<void, int> result = p(fake_context);
        EXPECT_EQ(1, run_count);
        EXPECT_EQ(fit::result_state::error, result.state());
        EXPECT_EQ(42, result.error());
        EXPECT_FALSE(p);
    }

    // Handler signature: fit::pending().
    {
        uint64_t run_count = 0;
        auto p = fit::make_promise([&] {
            run_count++;
            return fit::pending();
        });
        static_assert(std::is_same<void, decltype(p)::value_type>::value, "");
        static_assert(std::is_same<void, decltype(p)::error_type>::value, "");
        fit::result<> result = p(fake_context);
        EXPECT_EQ(1, run_count);
        EXPECT_EQ(fit::result_state::pending, result.state());
        EXPECT_TRUE(p);
    }

    // Handler signature: fit::promise_impl<...>.
    {
        uint64_t run_count = 0;
        uint64_t run_count2 = 0;
        auto p = fit::make_promise([&] {
            run_count++;
            return fit::make_promise([&]() -> fit::result<int, char> {
                if (++run_count2 == 2)
                    return fit::ok(42);
                return fit::pending();
            });
        });
        static_assert(std::is_same<int, decltype(p)::value_type>::value, "");
        static_assert(std::is_same<char, decltype(p)::error_type>::value, "");
        fit::result<int, char> result = p(fake_context);
        EXPECT_EQ(1, run_count);
        EXPECT_EQ(1, run_count2);
        EXPECT_EQ(fit::result_state::pending, result.state());
        EXPECT_TRUE(p);
        result = p(fake_context);
        EXPECT_EQ(1, run_count);
        EXPECT_EQ(2, run_count2);
        EXPECT_EQ(fit::result_state::ok, result.state());
        EXPECT_EQ(42, result.value());
        EXPECT_FALSE(p);
    }

    // Handler signature: void(context&).
    {
        uint64_t run_count = 0;
        auto p = fit::make_promise([&](fit::context& context) {
            ASSERT_CRITICAL(&context == &fake_context);
            run_count++;
        });
        static_assert(std::is_same<void, decltype(p)::value_type>::value, "");
        static_assert(std::is_same<void, decltype(p)::error_type>::value, "");
        fit::result<> result = p(fake_context);
        EXPECT_EQ(1, run_count);
        EXPECT_EQ(fit::result_state::ok, result.state());
        EXPECT_FALSE(p);
    }

    END_TEST;
}

// This is a bit lower level than fit::make_promise() in that there's
// no automatic adaptation of the handler type.
bool make_promise_with_continuation() {
    BEGIN_TEST;

    uint64_t run_count = 0;
    fake_context fake_context;
    auto p = fit::make_promise_with_continuation(
        [&](fit::context& context) -> fit::result<int, char> {
            ASSERT_CRITICAL(&context == &fake_context);
            run_count++;
            return fit::ok(42);
        });
    static_assert(std::is_same<int, decltype(p)::value_type>::value, "");
    static_assert(std::is_same<char, decltype(p)::error_type>::value, "");
    EXPECT_TRUE(p);

    fit::result<int, char> result = p(fake_context);
    EXPECT_EQ(1, run_count);
    EXPECT_EQ(fit::result_state::ok, result.state());
    EXPECT_EQ(42, result.value());
    EXPECT_FALSE(p);

    END_TEST;
}

auto make_ok_promise(int value) {
    return fit::make_promise([value, count = 0]() mutable -> fit::result<int, char> {
        ASSERT_CRITICAL(count == 0);
        ++count;
        return fit::ok(value);
    });
}

auto make_error_promise(char error) {
    return fit::make_promise([error, count = 0]() mutable -> fit::result<int, char> {
        ASSERT_CRITICAL(count == 0);
        ++count;
        return fit::error(error);
    });
}

auto make_delayed_ok_promise(int value) {
    return fit::make_promise([value, count = 0]() mutable -> fit::result<int, char> {
        ASSERT_CRITICAL(count <= 1);
        if (++count == 2)
            return fit::ok(value);
        return fit::pending();
    });
}

auto make_delayed_error_promise(char error) {
    return fit::make_promise([error, count = 0]() mutable -> fit::result<int, char> {
        ASSERT_CRITICAL(count <= 1);
        if (++count == 2)
            return fit::error(error);
        return fit::pending();
    });
}

// To keep these tests manageable, we only focus on argument type adaptation
// since return type adaptation logic is already covered by |make_promise()|
// and by the examples.
bool then_combinator() {
    BEGIN_TEST;

    fake_context fake_context;

    // Chaining on OK.
    // Handler signature: fit::result<>(fit::result<int, char>).
    {
        uint64_t run_count = 0;
        auto p =
            make_delayed_ok_promise(42)
                .then([&](fit::result<int, char> result) -> fit::result<> {
                    ASSERT_CRITICAL(result.value() == 42);
                    if (++run_count == 2)
                        return fit::ok();
                    return fit::pending();
                });

        fit::result<> result = p(fake_context);
        EXPECT_TRUE(p);
        EXPECT_EQ(0, run_count);
        EXPECT_EQ(fit::result_state::pending, result.state());

        result = p(fake_context);
        EXPECT_TRUE(p);
        EXPECT_EQ(1, run_count);
        EXPECT_EQ(fit::result_state::pending, result.state());

        result = p(fake_context);
        EXPECT_FALSE(p);
        EXPECT_EQ(2, run_count);
        EXPECT_EQ(fit::result_state::ok, result.state());
    }

    // Chaining on ERROR.
    // Handler signature: fit::result<>(fit::result<int, char>).
    {
        uint64_t run_count = 0;
        auto p =
            make_delayed_error_promise('x')
                .then([&](fit::result<int, char> result) -> fit::result<> {
                    ASSERT_CRITICAL(result.error() == 'x');
                    if (++run_count == 2)
                        return fit::ok();
                    return fit::pending();
                });

        fit::result<> result = p(fake_context);
        EXPECT_TRUE(p);
        EXPECT_EQ(0, run_count);
        EXPECT_EQ(fit::result_state::pending, result.state());

        result = p(fake_context);
        EXPECT_TRUE(p);
        EXPECT_EQ(1, run_count);
        EXPECT_EQ(fit::result_state::pending, result.state());

        result = p(fake_context);
        EXPECT_FALSE(p);
        EXPECT_EQ(2, run_count);
        EXPECT_EQ(fit::result_state::ok, result.state());
    }

    // Cover all handler argument signatures, more briefly.
    {
        uint64_t run_count = 0;
        auto p =
            make_ok_promise(42)
                .then([&](fit::result<int, char> result)
                          -> fit::result<int, char> {
                    run_count++;
                    return fit::ok(result.value() + 1);
                })
                .then([&](fit::result<int, char>& result)
                          -> fit::result<int, char> {
                    run_count++;
                    return fit::ok(result.value() + 1);
                })
                .then([&](const fit::result<int, char>& result)
                          -> fit::result<int, char> {
                    run_count++;
                    return fit::ok(result.value() + 1);
                })
                .then([&](fit::context& context, fit::result<int, char> result)
                          -> fit::result<int, char> {
                    ASSERT_CRITICAL(&context == &fake_context);
                    run_count++;
                    return fit::ok(result.value() + 1);
                })
                .then([&](fit::context& context, fit::result<int, char>& result)
                          -> fit::result<int, char> {
                    ASSERT_CRITICAL(&context == &fake_context);
                    run_count++;
                    return fit::ok(result.value() + 1);
                })
                .then([&](fit::context& context, const fit::result<int, char>& result)
                          -> fit::result<int, char> {
                    ASSERT_CRITICAL(&context == &fake_context);
                    run_count++;
                    return fit::ok(result.value() + 1);
                });

        fit::result<int, char> result = p(fake_context);
        EXPECT_FALSE(p);
        EXPECT_EQ(6, run_count);
        EXPECT_EQ(fit::result_state::ok, result.state());
        EXPECT_EQ(48, result.value());
    }

    END_TEST;
}

bool and_then_combinator() {
    BEGIN_TEST;

    fake_context fake_context;

    // Chaining on OK.
    // Handler signature: fit::result<>(int).
    {
        uint64_t run_count = 0;
        auto p =
            make_delayed_ok_promise(42)
                .and_then([&](int value) -> fit::result<void, char> {
                    ASSERT_CRITICAL(value == 42);
                    if (++run_count == 2)
                        return fit::error('y');
                    return fit::pending();
                });

        fit::result<void, char> result = p(fake_context);
        EXPECT_TRUE(p);
        EXPECT_EQ(0, run_count);
        EXPECT_EQ(fit::result_state::pending, result.state());

        result = p(fake_context);
        EXPECT_TRUE(p);
        EXPECT_EQ(1, run_count);
        EXPECT_EQ(fit::result_state::pending, result.state());

        result = p(fake_context);
        EXPECT_FALSE(p);
        EXPECT_EQ(2, run_count);
        EXPECT_EQ(fit::result_state::error, result.state());
        EXPECT_EQ('y', result.error());
    }

    // Chaining on ERROR.
    // Handler signature: fit::result<>(int).
    {
        uint64_t run_count = 0;
        auto p =
            make_delayed_error_promise('x')
                .and_then([&](int value) -> fit::result<void, char> {
                    run_count++;
                    return fit::pending();
                });

        fit::result<void, char> result = p(fake_context);
        EXPECT_TRUE(p);
        EXPECT_EQ(0, run_count);
        EXPECT_EQ(fit::result_state::pending, result.state());

        result = p(fake_context);
        EXPECT_FALSE(p);
        EXPECT_EQ(0, run_count);
        EXPECT_EQ(fit::result_state::error, result.state());
        EXPECT_EQ('x', result.error());
    }

    // Cover all handler argument signatures, more briefly.
    {
        uint64_t run_count = 0;
        auto p =
            make_ok_promise(42)
                .and_then([&](int value)
                              -> fit::result<int, char> {
                    run_count++;
                    return fit::ok(value + 1);
                })
                .and_then([&](int& value)
                              -> fit::result<int, char> {
                    run_count++;
                    return fit::ok(value + 1);
                })
                .and_then([&](const int& value)
                              -> fit::result<int, char> {
                    run_count++;
                    return fit::ok(value + 1);
                })
                .and_then([&](fit::context& context, int value)
                              -> fit::result<int, char> {
                    ASSERT_CRITICAL(&context == &fake_context);
                    run_count++;
                    return fit::ok(value + 1);
                })
                .and_then([&](fit::context& context, int& value)
                              -> fit::result<int, char> {
                    ASSERT_CRITICAL(&context == &fake_context);
                    run_count++;
                    return fit::ok(value + 1);
                })
                .and_then([&](fit::context& context, const int& value)
                              -> fit::result<int, char> {
                    ASSERT_CRITICAL(&context == &fake_context);
                    run_count++;
                    return fit::ok(value + 1);
                });

        fit::result<int, char> result = p(fake_context);
        EXPECT_EQ(6, run_count);
        EXPECT_EQ(fit::result_state::ok, result.state());
        EXPECT_EQ(48, result.value());
        EXPECT_FALSE(p);
    }

    END_TEST;
}

bool or_else_combinator() {
    BEGIN_TEST;

    fake_context fake_context;

    // Chaining on OK.
    // Handler signature: fit::result<>(char).
    {
        uint64_t run_count = 0;
        auto p =
            make_delayed_ok_promise(42)
                .or_else([&](char error) -> fit::result<int> {
                    run_count++;
                    return fit::pending();
                });

        fit::result<int> result = p(fake_context);
        EXPECT_TRUE(p);
        EXPECT_EQ(0, run_count);
        EXPECT_EQ(fit::result_state::pending, result.state());

        result = p(fake_context);
        EXPECT_FALSE(p);
        EXPECT_EQ(0, run_count);
        EXPECT_EQ(fit::result_state::ok, result.state());
        EXPECT_EQ(42, result.value());
    }

    // Chaining on ERROR.
    // Handler signature: fit::result<>(char).
    {
        uint64_t run_count = 0;
        auto p =
            make_delayed_error_promise('x')
                .or_else([&](char error) -> fit::result<int> {
                    ASSERT_CRITICAL(error == 'x');
                    if (++run_count == 2)
                        return fit::ok(43);
                    return fit::pending();
                });

        fit::result<int> result = p(fake_context);
        EXPECT_TRUE(p);
        EXPECT_EQ(0, run_count);
        EXPECT_EQ(fit::result_state::pending, result.state());

        result = p(fake_context);
        EXPECT_TRUE(p);
        EXPECT_EQ(1, run_count);
        EXPECT_EQ(fit::result_state::pending, result.state());

        result = p(fake_context);
        EXPECT_FALSE(p);
        EXPECT_EQ(2, run_count);
        EXPECT_EQ(fit::result_state::ok, result.state());
        EXPECT_EQ(43, result.value());
    }

    // Cover all handler argument signatures, more briefly.
    {
        uint64_t run_count = 0;
        auto p =
            make_error_promise('a')
                .or_else([&](char error)
                             -> fit::result<int, char> {
                    run_count++;
                    return fit::error(error + 1);
                })
                .or_else([&](char& error)
                             -> fit::result<int, char> {
                    run_count++;
                    return fit::error(error + 1);
                })
                .or_else([&](const char& error)
                             -> fit::result<int, char> {
                    run_count++;
                    return fit::error(error + 1);
                })
                .or_else([&](fit::context& context, char error)
                             -> fit::result<int, char> {
                    ASSERT_CRITICAL(&context == &fake_context);
                    run_count++;
                    return fit::error(error + 1);
                })
                .or_else([&](fit::context& context, char& error)
                             -> fit::result<int, char> {
                    ASSERT_CRITICAL(&context == &fake_context);
                    run_count++;
                    return fit::error(error + 1);
                })
                .or_else([&](fit::context& context, const char& error)
                             -> fit::result<int, char> {
                    ASSERT_CRITICAL(&context == &fake_context);
                    run_count++;
                    return fit::error(error + 1);
                });

        fit::result<int, char> result = p(fake_context);
        EXPECT_EQ(6, run_count);
        EXPECT_EQ(fit::result_state::error, result.state());
        EXPECT_EQ('g', result.error());
        EXPECT_FALSE(p);
    }

    END_TEST;
}

bool inspect_combinator() {
    BEGIN_TEST;

    fake_context fake_context;

    // Chaining on OK.
    // Handler signature: void(fit::result<int, char>).
    {
        uint64_t run_count = 0;
        auto p =
            make_delayed_ok_promise(42)
                .inspect([&](fit::result<int, char> result) {
                    ASSERT_CRITICAL(result.value() == 42);
                    run_count++;
                });

        fit::result<int, char> result = p(fake_context);
        EXPECT_TRUE(p);
        EXPECT_EQ(0, run_count);
        EXPECT_EQ(fit::result_state::pending, result.state());

        result = p(fake_context);
        EXPECT_FALSE(p);
        EXPECT_EQ(1, run_count);
        EXPECT_EQ(fit::result_state::ok, result.state());
        EXPECT_EQ(42, result.value());
    }

    // Chaining on ERROR.
    // Handler signature: void(fit::result<int, char>).
    {
        uint64_t run_count = 0;
        auto p =
            make_delayed_error_promise('x')
                .inspect([&](fit::result<int, char> result) {
                    ASSERT_CRITICAL(result.error() == 'x');
                    run_count++;
                });

        fit::result<int, char> result = p(fake_context);
        EXPECT_TRUE(p);
        EXPECT_EQ(0, run_count);
        EXPECT_EQ(fit::result_state::pending, result.state());

        result = p(fake_context);
        EXPECT_FALSE(p);
        EXPECT_EQ(1, run_count);
        EXPECT_EQ(fit::result_state::error, result.state());
        EXPECT_EQ('x', result.error());
    }

    // Cover all handler argument signatures, more briefly.
    {
        uint64_t run_count = 0;
        auto p =
            make_ok_promise(42)
                .inspect([&](fit::result<int, char> result) {
                    ASSERT_CRITICAL(result.value() == 42);
                    run_count++;
                })
                .inspect([&](fit::result<int, char>& result) {
                    ASSERT_CRITICAL(result.value() == 42);
                    run_count++;
                    result = fit::ok(result.value() + 1);
                })
                .inspect([&](const fit::result<int, char>& result) {
                    ASSERT_CRITICAL(result.value() == 43);
                    run_count++;
                })
                .inspect([&](fit::context& context, fit::result<int, char> result) {
                    ASSERT_CRITICAL(result.value() == 43);
                    ASSERT_CRITICAL(&context == &fake_context);
                    run_count++;
                })
                .inspect([&](fit::context& context, fit::result<int, char>& result) {
                    ASSERT_CRITICAL(result.value() == 43);
                    ASSERT_CRITICAL(&context == &fake_context);
                    run_count++;
                    result = fit::ok(result.value() + 1);
                })
                .inspect([&](fit::context& context, const fit::result<int, char>& result) {
                    ASSERT_CRITICAL(result.value() == 44);
                    ASSERT_CRITICAL(&context == &fake_context);
                    run_count++;
                });

        fit::result<int, char> result = p(fake_context);
        EXPECT_FALSE(p);
        EXPECT_EQ(6, run_count);
        EXPECT_EQ(fit::result_state::ok, result.state());
        EXPECT_EQ(44, result.value());
    }

    END_TEST;
}

bool discard_result_combinator() {
    BEGIN_TEST;

    fake_context fake_context;

    // Chaining on OK.
    {
        auto p = make_delayed_ok_promise(42).discard_result();
        static_assert(std::is_same<void, decltype(p)::value_type>::value, "");
        static_assert(std::is_same<void, decltype(p)::error_type>::value, "");

        fit::result<> result = p(fake_context);
        EXPECT_TRUE(p);
        EXPECT_EQ(fit::result_state::pending, result.state());

        result = p(fake_context);
        EXPECT_FALSE(p);
        EXPECT_EQ(fit::result_state::ok, result.state());
    }

    // Chaining on ERROR.
    {
        auto p = make_delayed_error_promise('x').discard_result();
        static_assert(std::is_same<void, decltype(p)::value_type>::value, "");
        static_assert(std::is_same<void, decltype(p)::error_type>::value, "");

        fit::result<> result = p(fake_context);
        EXPECT_TRUE(p);
        EXPECT_EQ(fit::result_state::pending, result.state());

        result = p(fake_context);
        EXPECT_FALSE(p);
        EXPECT_EQ(fit::result_state::ok, result.state());
    }

    END_TEST;
}

bool wrap_with_combinator() {
    BEGIN_TEST;

    fake_context fake_context;
    capture_result_wrapper<int, char> wrapper;
    uint64_t successor_run_count = 0;

    // Apply a wrapper which steals a promise's result th
    auto p = make_delayed_ok_promise(42)
                 .wrap_with(wrapper)
                 .then([&](fit::result<>) { successor_run_count++; });
    static_assert(std::is_same<void, decltype(p)::value_type>::value, "");
    static_assert(std::is_same<void, decltype(p)::error_type>::value, "");

    fit::result<> result = p(fake_context);
    EXPECT_TRUE(p);
    EXPECT_EQ(fit::result_state::pending, result.state());
    EXPECT_EQ(fit::result_state::pending, wrapper.last_result.state());
    EXPECT_EQ(0, successor_run_count);

    result = p(fake_context);
    EXPECT_FALSE(p);
    EXPECT_EQ(fit::result_state::ok, result.state());
    EXPECT_EQ(fit::result_state::ok, wrapper.last_result.state());
    EXPECT_EQ(42, wrapper.last_result.value());
    EXPECT_EQ(1, successor_run_count);

    END_TEST;
}

bool box_combinator() {
    BEGIN_TEST;

    fake_context fake_context;

    auto p = fit::make_promise([&]() -> fit::result<int, char> {
        return fit::ok(42);
    });
    static_assert(!std::is_same<fit::promise<int, char>, decltype(p)>::value, "");

    auto q = p.box();
    static_assert(std::is_same<fit::promise<int, char>, decltype(q)>::value, "");
    EXPECT_TRUE(q);
    EXPECT_FALSE(p);

    fit::result<int, char> result = q(fake_context);
    EXPECT_FALSE(q);
    EXPECT_EQ(fit::result_state::ok, result.state());
    EXPECT_EQ(42, result.value());

    END_TEST;
}

bool join_combinator() {
    BEGIN_TEST;

    fake_context fake_context;

    auto p = fit::join_promises(
        make_ok_promise(42),
        make_error_promise('x').or_else([](char error) {
            return fit::error('y');
        }),
        make_delayed_ok_promise(55));
    EXPECT_TRUE(p);

    fit::result<std::tuple<
        fit::result<int, char>,
        fit::result<int, char>,
        fit::result<int, char>>>
        result = p(fake_context);
    EXPECT_TRUE(p);
    EXPECT_EQ(fit::result_state::pending, result.state());

    result = p(fake_context);
    EXPECT_FALSE(p);
    EXPECT_EQ(fit::result_state::ok, result.state());
    EXPECT_EQ(42, std::get<0>(result.value()).value());
    EXPECT_EQ('y', std::get<1>(result.value()).error());
    EXPECT_EQ(55, std::get<2>(result.value()).value());

    END_TEST;
}

bool join_vector_combinator() {
    BEGIN_TEST;

    fake_context fake_context;

    std::vector<fit::promise<int, char>> promises;
    promises.push_back(make_ok_promise(42));
    promises.push_back(make_error_promise('x').or_else([](char error) {
        return fit::error('y');
    }));
    promises.push_back(make_delayed_ok_promise(55));
    auto p = fit::join_promise_vector(std::move(promises));
    EXPECT_TRUE(p);

    fit::result<std::vector<fit::result<int, char>>> result = p(fake_context);
    EXPECT_TRUE(p);
    EXPECT_EQ(fit::result_state::pending, result.state());

    result = p(fake_context);
    EXPECT_FALSE(p);
    EXPECT_EQ(fit::result_state::ok, result.state());
    EXPECT_EQ(42, result.value()[0].value());
    EXPECT_EQ('y', result.value()[1].error());
    EXPECT_EQ(55, result.value()[2].value());

    END_TEST;
}

// Ensure that fit::promise is considered nullable so that a promise can be
// directly stored as the continuation of another promise without any
// additional wrappers, similar to fit::function.
static_assert(fit::is_nullable<fit::promise<>>::value, "");

// Test return type adapation performed by handler invokers.
// These tests verify that the necessary specializations can be produced
// in all cases for handlers with various signatures.
namespace handler_invoker_test {

// handler returning void...
static_assert(std::is_same<
                  fit::result<>,
                  fit::internal::result_handler_invoker<
                      void (*)(fit::result<int, double>&),
                      fit::result<int, double>>::result_type>::value,
              "");
static_assert(std::is_same<
                  fit::result<void, double>,
                  fit::internal::value_handler_invoker<
                      void (*)(int&),
                      fit::result<int, double>>::result_type>::value,
              "");
static_assert(std::is_same<
                  fit::result<int, void>,
                  fit::internal::error_handler_invoker<
                      void (*)(double&),
                      fit::result<int, double>>::result_type>::value,
              "");

// handler returning fit::pending_result...
static_assert(std::is_same<
                  fit::result<>,
                  fit::internal::result_handler_invoker<
                      fit::pending_result (*)(fit::result<int, double>&),
                      fit::result<int, double>>::result_type>::value,
              "");
static_assert(std::is_same<
                  fit::result<void, double>,
                  fit::internal::value_handler_invoker<
                      fit::pending_result (*)(int&),
                      fit::result<int, double>>::result_type>::value,
              "");
static_assert(std::is_same<
                  fit::result<int, void>,
                  fit::internal::error_handler_invoker<
                      fit::pending_result (*)(double&),
                      fit::result<int, double>>::result_type>::value,
              "");

// handler returning fit::ok_result...
static_assert(std::is_same<
                  fit::result<unsigned, void>,
                  fit::internal::result_handler_invoker<
                      fit::ok_result<unsigned> (*)(fit::result<int, double>&),
                      fit::result<int, double>>::result_type>::value,
              "");
static_assert(std::is_same<
                  fit::result<unsigned, double>,
                  fit::internal::value_handler_invoker<
                      fit::ok_result<unsigned> (*)(int&),
                      fit::result<int, double>>::result_type>::value,
              "");
static_assert(std::is_same<
                  fit::result<int, void>,
                  fit::internal::error_handler_invoker<
                      fit::ok_result<int> (*)(double&),
                      fit::result<int, double>>::result_type>::value,
              "");

// handler returning fit::error_result...
static_assert(std::is_same<
                  fit::result<void, float>,
                  fit::internal::result_handler_invoker<
                      fit::error_result<float> (*)(fit::result<int, double>&),
                      fit::result<int, double>>::result_type>::value,
              "");
static_assert(std::is_same<
                  fit::result<void, double>,
                  fit::internal::value_handler_invoker<
                      fit::error_result<double> (*)(int&),
                      fit::result<int, double>>::result_type>::value,
              "");
static_assert(std::is_same<
                  fit::result<int, float>,
                  fit::internal::error_handler_invoker<
                      fit::error_result<float> (*)(double&),
                      fit::result<int, double>>::result_type>::value,
              "");

// handler returning fit::result...
static_assert(std::is_same<
                  fit::result<unsigned, float>,
                  fit::internal::result_handler_invoker<
                      fit::result<unsigned, float> (*)(fit::result<int, double>&),
                      fit::result<int, double>>::result_type>::value,
              "");
static_assert(std::is_same<
                  fit::result<unsigned, float>,
                  fit::internal::value_handler_invoker<
                      fit::result<unsigned, float> (*)(int&),
                      fit::result<int, double>>::result_type>::value,
              "");
static_assert(std::is_same<
                  fit::result<unsigned, float>,
                  fit::internal::error_handler_invoker<
                      fit::result<unsigned, float> (*)(double&),
                      fit::result<int, double>>::result_type>::value,
              "");

// handler returning fit::promise...
static_assert(std::is_same<
                  fit::result<unsigned, float>,
                  fit::internal::result_handler_invoker<
                      fit::promise<unsigned, float> (*)(fit::result<int, double>&),
                      fit::result<int, double>>::result_type>::value,
              "");
static_assert(std::is_same<
                  fit::result<unsigned, double>,
                  fit::internal::value_handler_invoker<
                      fit::promise<unsigned, double> (*)(int&),
                      fit::result<int, double>>::result_type>::value,
              "");
static_assert(std::is_same<
                  fit::result<int, float>,
                  fit::internal::error_handler_invoker<
                      fit::promise<int, float> (*)(double&),
                      fit::result<int, double>>::result_type>::value,
              "");

// handler returning lambda...
auto result_continuation_lambda = [](fit::result<int, double>&)
    -> fit::result<unsigned, float> { return fit::pending(); };
auto value_continuation_lambda = [](int&)
    -> fit::result<unsigned, double> { return fit::pending(); };
auto error_continuation_lambda = [](double&)
    -> fit::result<int, float> { return fit::pending(); };
static_assert(std::is_same<
                  fit::result<unsigned, float>,
                  fit::internal::result_handler_invoker<
                      decltype(result_continuation_lambda),
                      fit::result<int, double>>::result_type>::value,
              "");
static_assert(std::is_same<
                  fit::result<unsigned, double>,
                  fit::internal::value_handler_invoker<
                      decltype(value_continuation_lambda),
                      fit::result<int, double>>::result_type>::value,
              "");
static_assert(std::is_same<
                  fit::result<int, float>,
                  fit::internal::error_handler_invoker<
                      decltype(error_continuation_lambda),
                      fit::result<int, double>>::result_type>::value,
              "");

} // namespace handler_invoker_test

// Test predicate which is used interally to improve the quality of
// compilation errors when an invalid continuation type is encountered.
namespace is_continuation_test {

static_assert(fit::internal::is_continuation<
                  fit::function<fit::result<>(fit::context&)>>::value,
              "");
static_assert(!fit::internal::is_continuation<
                  fit::function<void(fit::context&)>>::value,
              "");
static_assert(!fit::internal::is_continuation<
                  fit::function<fit::result<>()>>::value,
              "");
static_assert(!fit::internal::is_continuation<
                  void>::value,
              "");

auto continuation_lambda = [](fit::context&)
    -> fit::result<> { return fit::pending(); };
auto invalid_lambda = [] {};

static_assert(fit::internal::is_continuation<
                  decltype(continuation_lambda)>::value,
              "");
static_assert(!fit::internal::is_continuation<
                  decltype(invalid_lambda)>::value,
              "");

} // namespace is_continuation_test
} // namespace

// These are compile-time diagnostic tests.
// We expect the following tests to fail at compile time and produce helpful
// static assertions when enabled manually.
#if 0
void diagnose_handler_with_invalid_return_type() {
    // Doesn't work because result isn't fit::result<>, fit::ok_result<>,
    // fit::error_result<>, fit::pending_result, a continuation, or void.
    fit::make_promise([]() -> int { return 0; });
}
#endif
#if 0
void diagnose_handler_with_too_few_arguments() {
    // Expected between 1 and 2 arguments, got 0.
    fit::make_promise([] {})
        .then([]() {});
}
#endif
#if 0
void diagnose_handler_with_too_many_arguments() {
    // Expected between 1 and 2 arguments, got 3.
    fit::make_promise([] {})
        .then([](fit::context&, fit::result<>, int excess) {});
}
#endif
#if 0
void diagnose_handler_with_invalid_context_arg() {
    // When there are two argument, the first must be fit::context&.
    fit::make_promise([] {})
        .then([](fit::result<>, int excess) {});
}
#endif
#if 0
void diagnose_handler_with_invalid_result_arg() {
    // The result type must match that produced by the prior.
    fit::make_promise([] {})
        .then([](fit::result<int>& result) {});
}
#endif
#if 0
void diagnose_handler_with_invalid_move_only_result_arg() {
    // Move-only types must be passed by reference not by value.
    fit::make_promise([] { return fit::ok(move_only{}); })
        .then([](fit::result<move_only> result) {});
}
#endif
#if 0
void diagnose_handler_with_invalid_value_arg() {
    // The value type must match that produced by the prior.
    fit::make_promise([] { return fit::ok(3.2f); })
        .and_then([](int value) {});
}
#endif
#if 0
void diagnose_handler_with_invalid_move_only_value_arg() {
    // The value type must match that produced by the prior.
    fit::make_promise([] { return fit::ok(move_only{}); })
        .and_then([](move_only value) {});
}
#endif
#if 0
void diagnose_handler_with_invalid_error_arg() {
    // The error type must match that produced by the prior.
    fit::make_promise([] { return fit::error(3.2f); })
        .or_else([](int error) {});
}
#endif
#if 0
void diagnose_handler_with_invalid_move_only_error_arg() {
    // The error type must match that produced by the prior.
    fit::make_promise([] { return fit::error(move_only{}); })
        .or_else([](move_only error) {});
}
#endif

BEGIN_TEST_CASE(promise_tests)
RUN_TEST(basics)
RUN_TEST(empty_promise)
RUN_TEST(invocation)
RUN_TEST(take_continuation)
RUN_TEST(assignment_and_swap)
RUN_TEST(comparison_with_nullptr)
RUN_TEST(make_promise)
RUN_TEST(make_promise_with_continuation)
RUN_TEST(then_combinator)
RUN_TEST(and_then_combinator)
RUN_TEST(or_else_combinator)
RUN_TEST(inspect_combinator)
RUN_TEST(discard_result_combinator)
RUN_TEST(wrap_with_combinator)
RUN_TEST(box_combinator)
RUN_TEST(join_combinator)
RUN_TEST(join_vector_combinator)

// suppress -Wunneeded-internal-declaration
(void)handler_invoker_test::result_continuation_lambda;
(void)handler_invoker_test::value_continuation_lambda;
(void)handler_invoker_test::error_continuation_lambda;
(void)is_continuation_test::continuation_lambda;
(void)is_continuation_test::invalid_lambda;
END_TEST_CASE(promise_tests)

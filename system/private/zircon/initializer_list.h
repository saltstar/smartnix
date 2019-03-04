
//
// Provides std::initializer_list<T> on behalf of the Zircon toolchain
// when compiling without the C++ standard library.
//

#pragma once

#ifndef ZIRCON_TOOLCHAIN
#error "This header must only be included when using the zircon toolchain."
#endif

// This implementation is known to be compatible with our GCC and Clang toolchains.
#if defined(__GNUC__) || defined(__clang__)

#include <stddef.h>

namespace std {
template <typename T>
class initializer_list {
public:
    using value_type = T;
    using reference = const T&;
    using const_reference = const T&;
    using size_type = size_t;
    using iterator = const T*;
    using const_iterator = const T*;

    constexpr initializer_list()
        : items_(nullptr), size_(0u) {}

    constexpr size_t size() const { return size_; }
    constexpr const T* begin() const { return items_; }
    constexpr const T* end() const { return items_ + size_; }

private:
    constexpr initializer_list(const T* items, size_t size)
        : items_(items), size_(size) {}

    const T* items_;
    size_t size_;
};
} // namespace std

#else
#error "std::initializer_list<T> not supported by this toolchain"
#endif // defined(__GNUC__) || defined(__clang__)

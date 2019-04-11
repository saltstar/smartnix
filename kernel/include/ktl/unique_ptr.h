
#pragma once

#include <fbl/alloc_checker.h>
#include <memory>

namespace ktl {

template <typename T, typename Deleter = ::std::default_delete<T>>
using unique_ptr = std::unique_ptr<T, Deleter>;

template <typename T, typename... Args>
unique_ptr<T> make_unique(fbl::AllocChecker* ac, Args&&... args) {
    return unique_ptr<T>(new (ac) T(std::forward<Args>(args)...));
}

} // namespace ktl

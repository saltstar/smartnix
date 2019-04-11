
#pragma once

#include <type_traits>

namespace ktl {

template <typename T>
constexpr typename std::remove_reference<T>::type&& move(T&& t) {
    return static_cast<typename std::remove_reference<T>::type&&>(t);
}

} // namespace ktl

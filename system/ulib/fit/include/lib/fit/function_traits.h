
#ifndef LIB_FIT_FUNCTION_TRAITS_H_
#define LIB_FIT_FUNCTION_TRAITS_H_

#include "traits.h"

namespace fit {

// function_traits is deprecated, please use callable_traits
template <typename T>
using function_traits = callable_traits<T>;

} // namespace fit

#endif // LIB_FIT_FUNCTION_TRAITS_H_

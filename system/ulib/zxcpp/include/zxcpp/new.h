
#pragma once

#include <stddef.h>

// Fake std::nothrow_t without a standard C++ library.
namespace std {
struct nothrow_t {};
}  // namespace std

// The kernel does not want non-AllocCheckered non-placement new
// overloads, but userspace can have them.
#if !_KERNEL
void* operator new(size_t);
void* operator new[](size_t);

#else // _KERNEL

void* operator new(size_t, void* caller, const std::nothrow_t&) noexcept;
void* operator new[](size_t, void* caller, const std::nothrow_t&) noexcept;

#endif // !_KERNEL

// Define placement new operators as inline for optimal code generation.
inline void* operator new(size_t, void *ptr) noexcept { return ptr; }
inline void* operator new[](size_t, void *ptr) noexcept { return ptr; }

void operator delete(void *p);
void operator delete[](void *p);
void operator delete(void *p, size_t);
void operator delete[](void *p, size_t);

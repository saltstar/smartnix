
#pragma once

#include <stddef.h>
#include <sys/types.h>
#include <zircon/compiler.h>

__BEGIN_CDECLS

// define this to enable collection of all unique call sites with unique sizes
#define HEAP_COLLECT_STATS 0

// standard heap definitions
void *malloc(size_t size) __MALLOC;
void *memalign(size_t boundary, size_t size) __MALLOC;
void *calloc(size_t count, size_t size) __MALLOC;
void *realloc(void *ptr, size_t size);
void free(void *ptr);

// inner function used when stats gathering is enabled
void *malloc_debug_caller_(size_t size, void *caller);

// alternate version of malloc where the caller is passed in
__MALLOC static inline void *malloc_debug_caller(size_t size, void *caller) {
    if (HEAP_COLLECT_STATS) {
        return malloc_debug_caller_(size, caller);
    } else {
        return malloc(size);
    }
}

// tell the heap to return any free pages it can find
void heap_trim(void);

// internal apis used by the heap implementation to get/return pages to the VM
void *heap_page_alloc(size_t pages);
void heap_page_free(void *ptr, size_t pages);

// Gets stats about the heap.
// |size_bytes| is the total size of the heap, |free_bytes| is the free portion.
void heap_get_info(size_t *size_bytes, size_t *free_bytes);

// called once at kernel initialization
void heap_init(void);

__END_CDECLS

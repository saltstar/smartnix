// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <stdlib.h>
#include <sys.h>
#include <bsp.h>

#ifndef CACHE_LINE_SIZE
#error bsp.h must define CACHE_LINE_SIZE
#endif

// Free allocated memory and clear pointer to it.
//
// Input: alloc_ptr_ptr = ptr to variable holding allocated address.
void free_clear(void* alloc_ptr_ptr) {
    void** allocpp = alloc_ptr_ptr;

    // Free the allocated memory.
    assert(*allocpp);
    free(*allocpp);

    // Clear the allocation pointer/flag.
    *allocpp = NULL;
}

// Allocate cache line size aligned memory.
//
// Input: size = amount of memory to allocate in bytes.
// Returns: Pointer to aligned memory block on success, else NULL.
void* aalloc(size_t size) {
#if CACHE_LINE_SIZE <= 8
    return malloc(size);
#else
    ui64 malloc_addr, fs_alloc_addr;

    // Increase size for malloc request to allow for alignment and for
    // storage of start of malloc-ed memory.
    size += sizeof(ui64) + CACHE_LINE_SIZE - 1;

    // Allocate memory.
    malloc_addr = (ui64)calloc(size, sizeof(ui8));
    if (malloc_addr == 0)
        return NULL;

    // Compute start of aligned memory block.
    fs_alloc_addr = (malloc_addr + sizeof(ui64) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);

    // Store start address immediately prior to aligned memory.
    *(ui64*)(fs_alloc_addr - sizeof(ui64)) = malloc_addr;

    // Return start of aligned memory.
    return (void*)fs_alloc_addr;
#endif
}

// Free allocated aligned memory and clear pointer to it.
//
// Input: aligned_ptr_addr = pointer to variable holding line-size aligned allocation address.
void afree_clear(void* aligned_ptr_addr) {
#if CACHE_LINE_SIZE <= 8
    free_clear(aligned_ptr_addr);
#else
    void*** aptr = aligned_ptr_addr;

    // Free allocated memory.
    free(*(*aptr - 1));

    // Clear input pointer.
    *aptr = 0;
#endif
}

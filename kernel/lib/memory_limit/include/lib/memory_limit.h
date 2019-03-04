
#pragma once
#include <iovec.h>
#include <sys/types.h>
#include <vm/pmm.h>
#include <zircon/types.h>

__BEGIN_CDECLS
// This library exists to calculate memory ranges to be used as arenas
// for the pmm based on a predefined memory limit. The limit is passed in
// MB via the kernel.memory-limit-mb cmdline argument. The library will
// calculate memory arenas based on provided ranges, reserved boot regions,
// and the limit provided and add those to the system when
// memory_limit_add_arenas is called.
//
// Checks if a memory limit exists and initializes the lib bookkeeping.
//
// Returns ZX_OK on success, ZX_ERR_BAD_STATE if already initialized, or
// ERR_NOT_SUPPORTED if no memory limit was passed via kernel.memory-limit-mb
zx_status_t memory_limit_init();

// Adds a given range of memory to the memory allocator to use in
// sorting out memory arenas.
//
// @range_base: the start address of the range.
// @range_size: size of the range in bytes
// @arena_template: a structure containing the default values for flags,
// priority, and name used for arenas created by this function in the
// event of any failure conditions.
//
// Returns ZX_OK on completion, and ZX_ERR_INVALID_ARGS if parameters are
// invalid
zx_status_t memory_limit_add_range(uintptr_t range_base,
                                   size_t range_size,
                                   pmm_arena_info_t arena_template);

// Uses the ranges provided by memory_limit_add_range to calculate the
// acceptable memory arenas to fit within our imposed memory limitations
// while still including all required reserved boot regions.
//
// @arena_template: a structure containing the default values for flags,
// priority, and name used for arenas created by this function in the
// event of any failure conditions.
zx_status_t memory_limit_add_arenas(pmm_arena_info_t arena_template);

__END_CDECLS

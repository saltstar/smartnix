
#pragma once

#include <assert.h>
#include <zircon/compiler.h>
#include <debug.h>
#include <err.h>
#include <sys/types.h>

__BEGIN_CDECLS

size_t hw_rng_get_entropy(void* buf, size_t len, bool block);

static inline uint32_t hw_rng_get_u32(void) {
    uint32_t ret;
    __UNUSED size_t fetched;

    fetched = hw_rng_get_entropy(&ret, sizeof(ret), true);
    DEBUG_ASSERT(fetched == sizeof(ret));
    return ret;
}

__END_CDECLS

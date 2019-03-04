
#pragma once

#include <zircon/compiler.h>
#include <zircon/types.h>

__BEGIN_CDECLS

/* This function is used by arch_copy_from_user() and arch_copy_to_user().
 * It should not be called anywhere except in the x86 usercopy
 * implementation. */

zx_status_t _x86_copy_to_or_from_user(
        void *dst,
        const void *src,
        size_t len,
        void **fault_return);

__END_CDECLS

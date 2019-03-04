
#include <zircon/syscalls.h>
#include <zircon/types.h>

#include <zircon/compiler.h>
#include "private.h"

zx_ticks_t _zx_ticks_per_second(void) {
    return DATA_CONSTANTS.ticks_per_second;
}

VDSO_INTERFACE_FUNCTION(zx_ticks_per_second);


#include <zircon/syscalls.h>

#include "private.h"

uint64_t _zx_system_get_physmem(void) {
    return DATA_CONSTANTS.physmem;
}

VDSO_INTERFACE_FUNCTION(zx_system_get_physmem);

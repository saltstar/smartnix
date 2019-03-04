
#include <zircon/syscalls.h>

#include <zircon/compiler.h>
#include "private.h"

uint32_t _zx_system_get_dcache_line_size(void) {
    return DATA_CONSTANTS.dcache_line_size;
}

VDSO_INTERFACE_FUNCTION(zx_system_get_dcache_line_size);

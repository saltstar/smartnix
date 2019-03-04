
#include <zircon/syscalls.h>

#include <zircon/compiler.h>
#include "private.h"

uint32_t _zx_system_get_num_cpus(void) {
    return DATA_CONSTANTS.max_num_cpus;
}

VDSO_INTERFACE_FUNCTION(zx_system_get_num_cpus);

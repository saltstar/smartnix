
#include <zircon/syscalls.h>
#include <zircon/time.h>

#include "private.h"

zx_time_t _zx_deadline_after(zx_duration_t nanoseconds) {
    zx_time_t now = VDSO_zx_clock_get_monotonic();
    return zx_time_add_duration(now, nanoseconds);
}

VDSO_INTERFACE_FUNCTION(zx_deadline_after);

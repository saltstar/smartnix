
#pragma once

#include <sys/types.h>
#include <zircon/compiler.h>
#include <zircon/types.h>

__BEGIN_CDECLS

zx_time_t cntpct_to_zx_time(uint64_t cntpct);

__END_CDECLS

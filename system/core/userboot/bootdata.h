
#pragma once

#pragma GCC visibility push(hidden)

#include <zircon/types.h>

zx_handle_t bootdata_get_bootfs(zx_handle_t log, zx_handle_t vmar_self,
                                zx_handle_t bootdata_vmo);

#pragma GCC visibility pop

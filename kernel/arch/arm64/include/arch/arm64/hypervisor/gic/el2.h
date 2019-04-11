
#pragma once

#include <zircon/compiler.h>
#include <zircon/types.h>

__BEGIN_CDECLS

extern void arm64_el2_gicv3_read_gich_state(zx_paddr_t state);
extern void arm64_el2_gicv3_write_gich_state(zx_paddr_t state, uint32_t hcr);
extern uint32_t arm64_el2_gicv3_read_gich_vtr();

__END_CDECLS

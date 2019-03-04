
#pragma once

#include <zircon/compiler.h>
#include <arch/defines.h>
#include <zircon/types.h>

__BEGIN_CDECLS

extern uint32_t arm64_el2_gicv3_read_gich_hcr();
extern void arm64_el2_gicv3_write_gich_hcr(uint32_t val);
extern uint32_t arm64_el2_gicv3_read_gich_vtr();
extern uint32_t arm64_el2_gicv3_read_gich_vmcr();
extern void arm64_el2_gicv3_write_gich_vmcr(uint32_t val);
extern uint32_t arm64_el2_gicv3_read_gich_misr();
extern uint32_t arm64_el2_gicv3_read_gich_elrsr();
extern uint32_t arm64_el2_gicv3_read_gich_apr();
extern void arm64_el2_gicv3_write_gich_apr(uint32_t val);
extern uint64_t arm64_el2_gicv3_read_gich_lr(uint32_t index);
extern void arm64_el2_gicv3_write_gich_lr(uint64_t val, uint32_t index);

__END_CDECLS

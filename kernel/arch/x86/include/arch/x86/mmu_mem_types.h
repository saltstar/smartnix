
#pragma once

#include <kernel/cpu.h>
#include <zircon/compiler.h>

__BEGIN_CDECLS

void x86_mmu_mem_type_init(void);
void x86_pat_sync(cpu_mask_t targets);

__END_CDECLS

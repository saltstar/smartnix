
#pragma once

// clang-format off

#define GICH_LR_VIRTUAL_ID_MASK   0x3ff
#define GICH_LR_PENDING           (1u << 28)
#define GICH_VMCR_VENG0           (1u << 0)
#define GICH_VMCR_VPMR_MASK       (0x1fu << 27)
#define GICH_VTR_LIST_REGS_MASK   0x3f

// clang-format on

void gicv2_hw_interface_register();

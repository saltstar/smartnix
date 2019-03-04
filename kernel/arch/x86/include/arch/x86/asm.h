
#pragma once

#include <asm.h>

/* x86 assembly macros used in a few files */

#define PHYS_LOAD_ADDRESS (KERNEL_LOAD_OFFSET)
#define PHYS_ADDR_DELTA (KERNEL_BASE - PHYS_LOAD_ADDRESS)
#define PHYS(x) ((x) - PHYS_ADDR_DELTA)


#pragma once

#include <zircon/boot/image.h>

// This symbol is defined in boot-shim.ld.
extern zircon_kernel_t embedded_zbi;

// This type is tailored for the ARM64 C ABI returning to assembly code.
typedef struct {
    zbi_header_t* zbi;                  // Returned in x0.
    uint64_t entry;                     // Returned in x1.
} boot_shim_return_t;

boot_shim_return_t boot_shim(void* device_tree);

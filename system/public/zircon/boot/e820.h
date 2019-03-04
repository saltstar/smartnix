
#pragma once

#include <stdint.h>
#include <zircon/compiler.h>

#define E820_RAM 1
#define E820_RESERVED 2
#define E820_ACPI 3
#define E820_NVS 4
#define E820_UNUSABLE 5

typedef struct e820entry {
    uint64_t addr;
    uint64_t size;
    uint32_t type;
} __PACKED e820entry_t;

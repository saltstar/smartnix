
#pragma once

#include <zircon/hw/gpt.h>

typedef struct guid_map {
    const char name[GPT_NAME_LEN];
    const uint8_t guid[GPT_GUID_LEN];
} guid_map_t;

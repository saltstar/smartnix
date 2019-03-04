
#pragma once

#include <ddk/device.h>
#include <ddk/protocol/platform-device.h>

typedef struct {
    zx_device_t* zxdev;
    pdev_protocol_t pdev;
    // more stuff will be added here
} gauss_pdm_input_t;

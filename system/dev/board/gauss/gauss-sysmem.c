// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ddk/debug.h>
#include <ddk/device.h>
#include <ddk/platform-defs.h>
#include <ddk/protocol/platform/bus.h>
#include "gauss.h"

static const pbus_bti_t sysmem_btis[] = {
    {
        .iommu_index = 0,
        .bti_id = BTI_SYSMEM,
    },
};

static const pbus_dev_t sysmem_dev = {
    .name = "sysmem",
    .vid = PDEV_VID_GENERIC,
    .pid = PDEV_PID_GENERIC,
    .did = PDEV_DID_SYSMEM,
    .bti_list = sysmem_btis,
    .bti_count = countof(sysmem_btis),
};

zx_status_t gauss_sysmem_init(gauss_bus_t* bus) {
    zx_status_t status;

    if ((status = pbus_protocol_device_add(&bus->pbus, ZX_PROTOCOL_SYSMEM, &sysmem_dev)) != ZX_OK) {
        zxlogf(ERROR, "gauss_sysmem_init: pbus_protocol_device_add() failed for sysmem: %d\n", status);
        return status;
    }

    return ZX_OK;
}

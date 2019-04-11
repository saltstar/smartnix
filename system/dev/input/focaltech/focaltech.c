// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ddk/binding.h>
#include <ddk/driver.h>
#include <ddk/platform-defs.h>

extern zx_status_t ft_device_bind(void* ctx, zx_device_t* parent);

static zx_driver_ops_t focaltech_driver_ops = {
    .version = DRIVER_OPS_VERSION,
    .bind = ft_device_bind,
};

// clang-format off
ZIRCON_DRIVER_BEGIN(focaltech_touch, focaltech_driver_ops, "focaltech-touch", "0.1", 3)
    BI_ABORT_IF(NE, BIND_PROTOCOL, ZX_PROTOCOL_PDEV),
    BI_ABORT_IF(NE, BIND_PLATFORM_DEV_VID, PDEV_VID_GENERIC),
    BI_MATCH_IF(EQ, BIND_PLATFORM_DEV_DID, PDEV_DID_FOCALTOUCH),
ZIRCON_DRIVER_END(focaltech_touch)
// clang-format on

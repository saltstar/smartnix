// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <lib/zx/bti.h>
#include <zircon/compiler.h>
#include <ddk/protocol/platform/device.h>
#include <ddk/protocol/platform-device-lib.h>
#include <zircon/assert.h>
#include <ddktl/device.h>
#include <ddktl/mmio.h>
#include <fbl/unique_ptr.h>
#include <optional>
#include "common.h"
#include "registers-disp-rdma.h"

namespace mt8167s_display {

// [Ovl] --> [Clr] --> [Clr Correction] --> [AAL] --> [Gamma] --> [Dither] --> [RDMA] --> [DSI]

//
// Display RDMA is responsible for performing DMA operations from the display pipelines and
// outputting it to the DSI module. RDMA can work in two mode: Memory and Direct. For now,
// we will configure RDMA to work in Direct Link mode which basically uses the memory already
// being used by the Overlay engine. RDMA engine is also capable of inline conversion from
// RGB to YUV space and vice versa. However, since we currently operate in RGB mode, we don't
// need to use that.
// Note: There are multiple RDMA engines within the display subsystem. There is internal RDMA
// in the overlay engine. There is RDMA engine that perform mem-2-mem operations when performing
// tasks such as rotation or scaling. There is also RDMA engine that transfers content from
// display subsytem to the DSI engine.
//

class DispRdma {
public:
    DispRdma(uint32_t height, uint32_t width) : height_(height), width_(width) {
        ZX_ASSERT(height_ < kMaxHeight);
        ZX_ASSERT(width_ < kMaxWidth);
    }

    // Init
    zx_status_t Init(zx_device_t* parent);

    // Reset Display RDMA engine
    void Reset();

    // Start the Display RDMA engine
    void Start();

    // Stop the Display RDMA engine
    void Stop();

    void Restart() {
        Stop();
        Start();
    }

    // Configure Display RDMA engine based on display dimensions
    zx_status_t Config();

    // Dumps all the relevant Display Registers
    void Dump();

private:
    uint32_t                            height_;
    uint32_t                            width_;

    fbl::unique_ptr<ddk::MmioBuffer>    disp_rdma_mmio_;
    pdev_protocol_t                     pdev_ = {nullptr, nullptr};
    zx::bti                             bti_;

    bool                                initialized_ = false;
};

} // namespace mt8167s_display
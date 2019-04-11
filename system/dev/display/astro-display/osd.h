// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <zircon/compiler.h>
#include <ddk/protocol/platform/device.h>
#include <ddk/protocol/platform-device-lib.h>
#include <ddktl/mmio.h>
#include <lib/zx/interrupt.h>
#include <lib/zx/bti.h>
#include <threads.h>
#include "common.h"
#include <optional>

namespace astro_display {

struct RdmaTable {
    uint32_t reg;
    uint32_t val;
};

enum {
    IDX_CFG_W0,
    IDX_CTRL_STAT,
    IDX_MAX,
};

struct RdmaChannelContainer {
    zx_paddr_t      phys_offset;    // offset into physical address
    uint8_t*        virt_offset;    // offset into virtual address (vmar buf)
    bool            active;         // indicated whether channel is being used or not
};

constexpr uint32_t kRdmaTableMaxSize = IDX_MAX;

// RDMA Channels used by OSD. Three channels should be more than enough
constexpr uint8_t kMaxRdmaChannels = 3;
constexpr uint8_t kMaxRetries = 100;
// spread channels 512B apart (make sure it's greater than a cache line size)
constexpr size_t  kChannelBaseOffset = 512;

class Osd {
public:
    Osd(uint32_t fb_width, uint32_t fb_height, uint32_t display_width, uint32_t display_height)
        : fb_width_(fb_width), fb_height_(fb_height),
          display_width_(display_width), display_height_(display_height) {}

    zx_status_t Init(zx_device_t* parent);
    void HwInit();
    zx_status_t Configure();
    void Disable();
    // This function will apply configuration when VSYNC interrupt occurs using RDMA
    void FlipOnVsync(uint8_t idx);
    void Dump();
    void Release();

private:
    void DefaultSetup();
    // this function sets up scaling based on framebuffer and actual display
    // dimensions. The scaling IP and registers and undocumented.
    void EnableScaling(bool enable);
    void Enable();
    zx_status_t SetupRdma();
    void ResetRdmaTable();
    void SetRdmaTableValue(uint32_t channel, uint32_t idx, uint32_t val);
    void FlushRdmaTable(uint32_t channel);
    int GetNextAvailableRdmaChannel();
    int RdmaThread();

    std::optional<ddk::MmioBuffer>      vpu_mmio_;
    pdev_protocol_t                     pdev_ = {nullptr, nullptr};
    zx::bti                             bti_;

    // RDMA IRQ handle and thread
    zx::interrupt                       rdma_irq_;
    thrd_t                              rdma_thread_;

    // use a single vmo for all channels
    zx::vmo                             rdma_vmo_;
    zx_handle_t                         rdma_pmt_;
    zx_paddr_t                          rdma_phys_;
    uint8_t*                            rdma_vbuf_;

    // container that holds channel specific properties
    RdmaChannelContainer               rdma_chnl_container_[kMaxRdmaChannels];

    // Framebuffer dimension
    uint32_t                            fb_width_;
    uint32_t                            fb_height_;
    // Actual display dimension
    uint32_t                            display_width_;
    uint32_t                            display_height_;

    bool                                initialized_ = false;
};

} // namespace astro_display

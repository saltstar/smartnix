// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ddk/debug.h>
#include <ddk/device.h>
#include <ddk/metadata.h>
#include <ddk/metadata/gpt.h>
#include <ddk/platform-defs.h>
#include <ddk/protocol/platform/bus.h>
#include <fbl/algorithm.h>
#include <hwreg/bitfields.h>
#include <soc/mt8167/mt8167-hw.h>
#include <soc/mt8167/mt8167-sdmmc.h>

#include "mt8167.h"

namespace {

constexpr uintptr_t kClkBaseAligned =
    fbl::round_down<uintptr_t, uintptr_t>(MT8167_XO_BASE, PAGE_SIZE);
constexpr size_t kClkOffset = MT8167_XO_BASE - kClkBaseAligned;
constexpr size_t kClkSizeAligned =
    fbl::round_up<size_t, size_t>(kClkOffset + MT8167_XO_SIZE, PAGE_SIZE);

constexpr uintptr_t kPllBaseAligned =
    fbl::round_down<uintptr_t, uintptr_t>(MT8167_AP_MIXED_SYS_BASE, PAGE_SIZE);
constexpr size_t kPllOffset = MT8167_AP_MIXED_SYS_BASE - kPllBaseAligned;
constexpr size_t kPllSizeAligned =
    fbl::round_up<size_t, size_t>(kPllOffset + MT8167_AP_MIXED_SYS_SIZE, PAGE_SIZE);

// MMPLL is derived from the 26 MHz crystal oscillator.
constexpr uint32_t kMmPllSrcClkFreq = 26000000;

constexpr uint32_t kFifoDepth = 128;
constexpr uint32_t kSrcClkFreq = 200000000;

}  // namespace

namespace board_mt8167 {

class ClkMuxSel0 : public hwreg::RegisterBase<ClkMuxSel0, uint32_t> {
public:
    static constexpr uint32_t kClkMmPllDiv2 = 7;

    static auto Get() { return hwreg::RegisterAddr<ClkMuxSel0>(kClkOffset); }

    DEF_FIELD(13, 11, msdc0_mux_sel);
};

class MmPllCon1 : public hwreg::RegisterBase<MmPllCon1, uint32_t> {
public:
    static constexpr uint32_t kDiv1  = 0;
    static constexpr uint32_t kDiv2  = 1;
    static constexpr uint32_t kDiv4  = 2;
    static constexpr uint32_t kDiv8  = 3;
    static constexpr uint32_t kDiv16 = 4;

    static constexpr uint32_t kPcwFracBits = 14;

    static auto Get() { return hwreg::RegisterAddr<MmPllCon1>(kPllOffset + 0x164); }

    DEF_BIT(31, change);
    DEF_FIELD(26, 24, div);
    DEF_FIELD(20, 0, pcw);
};

zx_status_t Mt8167::EmmcInit() {
    static const pbus_mmio_t emmc_mmios[] = {
        {
            .base = MT8167_MSDC0_BASE,
            .length = MT8167_MSDC0_SIZE,
        }
    };

    static const pbus_bti_t emmc_btis[] = {
        {
            .iommu_index = 0,
            .bti_id = BTI_EMMC,
        }
    };

    static const MtkSdmmcConfig emmc_config = {
        .fifo_depth = kFifoDepth,
        .src_clk_freq = kSrcClkFreq
    };

    static const guid_map_t guid_map[] = {
        { "boot_a", GUID_ZIRCON_A_VALUE },
        { "boot_b", GUID_ZIRCON_B_VALUE },
        { "vbmeta_a", GUID_VBMETA_A_VALUE },
        { "vbmeta_b", GUID_VBMETA_B_VALUE },
        { "userdata", GUID_FVM_VALUE },
        // For now, just give the paver a place to write Zircon-R,
        // even though the bootloader won't support it.
        { "vendor_a", GUID_ZIRCON_R_VALUE },
    };
    static_assert(fbl::count_of(guid_map) <= DEVICE_METADATA_GUID_MAP_MAX_ENTRIES);

    static const pbus_metadata_t emmc_metadata[] = {
        {
            .type = DEVICE_METADATA_PRIVATE,
            .data_buffer = &emmc_config,
            .data_size = sizeof(emmc_config)
        },
        {
            .type = DEVICE_METADATA_GUID_MAP,
            .data_buffer = guid_map,
            .data_size = sizeof(guid_map)
        }
    };

    static const pbus_irq_t emmc_irqs[] = {
        {
            .irq = MT8167_IRQ_MSDC0,
            .mode = ZX_INTERRUPT_MODE_EDGE_HIGH
        }
    };

    static const pbus_gpio_t emmc_gpios[] = {
        {
            .gpio = MT8167_GPIO_MSDC0_RST
        }
    };

    pbus_dev_t emmc_dev = {};
    emmc_dev.name = "emmc";
    emmc_dev.vid = PDEV_VID_MEDIATEK;
    emmc_dev.did = PDEV_DID_MEDIATEK_EMMC;
    emmc_dev.mmio_list = emmc_mmios;
    emmc_dev.mmio_count = countof(emmc_mmios);
    emmc_dev.bti_list = emmc_btis;
    emmc_dev.bti_count = countof(emmc_btis);
    emmc_dev.metadata_list = emmc_metadata;
    emmc_dev.metadata_count = countof(emmc_metadata);
    emmc_dev.irq_list = emmc_irqs;
    emmc_dev.irq_count = countof(emmc_irqs);
    emmc_dev.gpio_list = emmc_gpios;
    emmc_dev.gpio_count = countof(emmc_gpios);

    // TODO(bradenkell): Have the clock driver do this once muxing is supported.
    zx::unowned_resource root_resource(get_root_resource());
    std::optional<ddk::MmioBuffer> clk_mmio;
    zx_status_t status = ddk::MmioBuffer::Create(kClkBaseAligned, kClkSizeAligned, *root_resource,
                                                 ZX_CACHE_POLICY_UNCACHED_DEVICE, &clk_mmio);
    if (status != ZX_OK) {
        zxlogf(ERROR, "%s: Failed to set MSDC0 clock: %d\n", __FUNCTION__, status);
        return status;
    }

    std::optional<ddk::MmioBuffer> pll_mmio;
    status = ddk::MmioBuffer::Create(kPllBaseAligned, kPllSizeAligned, *root_resource,
                                     ZX_CACHE_POLICY_UNCACHED_DEVICE, &pll_mmio);
    if (status != ZX_OK) {
        zxlogf(ERROR, "%s: Failed to set MSDC0 clock: %d\n", __FUNCTION__, status);
        return status;
    }

    constexpr uint32_t div_value = MmPllCon1::kDiv4;
    // The MSDC0 clock will be set to MMPLL/2, so shift an extra bit to get 400 MHz.
    constexpr uint32_t src_clk_shift = 1 + MmPllCon1::kPcwFracBits + div_value;
    constexpr uint64_t pcw =
        (static_cast<uint64_t>(kSrcClkFreq) << src_clk_shift) / kMmPllSrcClkFreq;
    MmPllCon1::Get()
        .ReadFrom(&(*pll_mmio))
        .set_change(1)
        .set_div(div_value)
        .set_pcw(pcw)
        .WriteTo(&(*pll_mmio));

    ClkMuxSel0::Get()
        .ReadFrom(&(*clk_mmio))
        .set_msdc0_mux_sel(ClkMuxSel0::kClkMmPllDiv2)
        .WriteTo(&(*clk_mmio));

    if ((status = pbus_.DeviceAdd(&emmc_dev)) != ZX_OK) {
        zxlogf(ERROR, "%s: DeviceAdd MSDC0 failed: %d\n", __FUNCTION__, status);
    }

    return status;
}

} // namespace board_mt8167

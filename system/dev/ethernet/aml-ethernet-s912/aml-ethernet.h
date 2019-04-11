// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ddk/device.h>
#include <ddktl/device.h>
#include <ddktl/i2c-channel.h>
#include <ddktl/mmio.h>
#include <ddktl/pdev.h>
#include <ddktl/protocol/ethernet/board.h>
#include <ddktl/protocol/gpio.h>
#include <threads.h>

#include <optional>

namespace eth {

class AmlEthernet;
using DeviceType = ddk::Device<AmlEthernet, ddk::Unbindable>;

class AmlEthernet : public DeviceType,
                    public ddk::EthBoardProtocol<AmlEthernet, ddk::base_protocol> {
public:
    DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(AmlEthernet);

    explicit AmlEthernet(zx_device_t* parent)
        : DeviceType(parent), pdev_(parent) {}

    static zx_status_t Create(zx_device_t* parent);

    // DDK Hooks.
    void DdkRelease();
    void DdkUnbind();

    // ETH_BOARD protocol.
    void EthBoardResetPhy();

private:
    // GPIO Indexes.
    enum {
        PHY_RESET,
        PHY_INTR,
        GPIO_COUNT,
    };

    // MMIO Indexes
    enum {
        MMIO_PERIPH,
        MMIO_HHI,
    };

    zx_status_t InitPdev();
    zx_status_t Bind();

    ddk::PDev pdev_;
    ddk::I2cChannel i2c_;
    ddk::GpioProtocolClient gpios_[GPIO_COUNT];

    std::optional<ddk::MmioBuffer> periph_mmio_;
    std::optional<ddk::MmioBuffer> hhi_mmio_;
};

} // namespace eth

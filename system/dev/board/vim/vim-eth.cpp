// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ddk/debug.h>
#include <ddk/device.h>
#include <ddk/metadata.h>
#include <ddk/platform-defs.h>
#include <ddk/protocol/ethernet.h>
#include <soc/aml-s912/s912-gpio.h>
#include <soc/aml-s912/s912-hw.h>

#include <limits.h>

#include "vim.h"
namespace vim {
static const pbus_gpio_t eth_board_gpios[] = {
    {
        // MAC_RST
        .gpio = S912_GPIOZ(14),
    },
    {
        // MAC_INTR (need to wire up interrupt?)
        .gpio = S912_GPIOZ(15),
    },
};

static const pbus_irq_t eth_mac_irqs[] = {
    {
        .irq = S912_ETH_GMAC_IRQ,
        .mode = ZX_INTERRUPT_MODE_EDGE_HIGH,
    },
};

static const pbus_mmio_t eth_board_mmios[] = {
    {
        .base = PERIPHS_REG_BASE,
        .length = PERIPHS_REG_SIZE,
    },
    {
        .base = HHI_REG_BASE,
        .length = HHI_REG_SIZE,
    },
};

static const pbus_mmio_t eth_mac_mmios[] = {
    {
        .base = ETH_MAC_REG_BASE,
        .length = ETH_MAC_REG_SIZE,
    },
};

static const pbus_bti_t eth_mac_btis[] = {
    {
        .iommu_index = 0,
        .bti_id = 0,
    },
};

static const pbus_boot_metadata_t eth_mac_metadata[] = {
    {
        .zbi_type = DEVICE_METADATA_MAC_ADDRESS,
        .zbi_extra = 0,
    },
};

static const eth_dev_metadata_t eth_phy_device = {
    .vid = PDEV_VID_REALTEK,
    .pid = PDEV_PID_RTL8211F,
    .did = PDEV_DID_ETH_PHY,
};

static const pbus_metadata_t eth_mac_device_metadata[] = {
    {
        .type = DEVICE_METADATA_PRIVATE,
        .data_buffer = &eth_phy_device,
        .data_size = sizeof(eth_dev_metadata_t),
    },
};

static const eth_dev_metadata_t eth_mac_device = {
    .vid = PDEV_VID_DESIGNWARE,
    //c++ init error
    .pid = 0,
    //c++ init error
    .did = PDEV_DID_ETH_MAC,
};

static const pbus_metadata_t eth_board_metadata[] = {
    {
        .type = DEVICE_METADATA_PRIVATE,
        .data_buffer = &eth_mac_device,
        .data_size = sizeof(eth_dev_metadata_t),
    },
};

static const pbus_i2c_channel_t vim2_mcu_i2c[] = {
    {
        .bus_id = 1,
        .address = 0x18,
    },
};

static const pbus_dev_t eth_board_children[] = {
    // Designware MAC.
    []() {
        pbus_dev_t dev;
        dev.name = "dwmac";
        dev.mmio_list = eth_mac_mmios;
        dev.mmio_count = countof(eth_mac_mmios);
        dev.irq_list = eth_mac_irqs;
        dev.irq_count = countof(eth_mac_irqs);
        dev.bti_list = eth_mac_btis;
        dev.bti_count = countof(eth_mac_btis);
        dev.metadata_list = eth_mac_device_metadata;
        dev.metadata_count = countof(eth_mac_device_metadata);
        dev.boot_metadata_list = eth_mac_metadata;
        dev.boot_metadata_count = countof(eth_mac_metadata);
        return dev;
    }(),
};

zx_status_t Vim::EthInit() {

    pbus_dev_t eth_board_dev = {};
    eth_board_dev.name = "ethernet_mac";
    eth_board_dev.vid = PDEV_VID_KHADAS;
    eth_board_dev.pid = PDEV_PID_VIM2;
    eth_board_dev.did = PDEV_DID_AMLOGIC_ETH;
    eth_board_dev.mmio_list = eth_board_mmios;
    eth_board_dev.mmio_count = countof(eth_board_mmios);
    eth_board_dev.gpio_list = eth_board_gpios;
    eth_board_dev.gpio_count = countof(eth_board_gpios);
    eth_board_dev.i2c_channel_list = vim2_mcu_i2c;
    eth_board_dev.i2c_channel_count = countof(vim2_mcu_i2c);
    eth_board_dev.metadata_list = eth_board_metadata;
    eth_board_dev.metadata_count = countof(eth_board_metadata);
    eth_board_dev.child_list = eth_board_children;
    eth_board_dev.child_count = countof(eth_board_children);

    // setup pinmux for RGMII connections
    gpio_impl_.SetAltFunction(S912_ETH_MDIO, S912_ETH_MDIO_FN);
    gpio_impl_.SetAltFunction(S912_ETH_MDC, S912_ETH_MDC_FN);
    gpio_impl_.SetAltFunction(S912_ETH_RGMII_RX_CLK,
                              S912_ETH_RGMII_RX_CLK_FN);
    gpio_impl_.SetAltFunction(S912_ETH_RX_DV, S912_ETH_RX_DV_FN);
    gpio_impl_.SetAltFunction(S912_ETH_RXD0, S912_ETH_RXD0_FN);
    gpio_impl_.SetAltFunction(S912_ETH_RXD1, S912_ETH_RXD1_FN);
    gpio_impl_.SetAltFunction(S912_ETH_RXD2, S912_ETH_RXD2_FN);
    gpio_impl_.SetAltFunction(S912_ETH_RXD3, S912_ETH_RXD3_FN);

    gpio_impl_.SetAltFunction(S912_ETH_RGMII_TX_CLK,
                              S912_ETH_RGMII_TX_CLK_FN);
    gpio_impl_.SetAltFunction(S912_ETH_TX_EN, S912_ETH_TX_EN_FN);
    gpio_impl_.SetAltFunction(S912_ETH_TXD0, S912_ETH_TXD0_FN);
    gpio_impl_.SetAltFunction(S912_ETH_TXD1, S912_ETH_TXD1_FN);
    gpio_impl_.SetAltFunction(S912_ETH_TXD2, S912_ETH_TXD2_FN);
    gpio_impl_.SetAltFunction(S912_ETH_TXD3, S912_ETH_TXD3_FN);

    zx_status_t status = pbus_.DeviceAdd(&eth_board_dev);

    if (status != ZX_OK) {
        zxlogf(ERROR, "EthInit: pbus_device_add failed: %d\n", status);
    }
    return status;
}
} //namespace vim
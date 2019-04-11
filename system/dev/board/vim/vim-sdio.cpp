// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ddk/debug.h>
#include <ddk/metadata.h>
#include <ddk/platform-defs.h>
#include <hw/reg.h>
#include <soc/aml-a113/a113-hw.h>
#include <soc/aml-common/aml-sd-emmc.h>
#include <soc/aml-s912/s912-gpio.h>
#include <soc/aml-s912/s912-hw.h>
#include <wifi/wifi-config.h>

#include "vim.h"

namespace vim {
static const pbus_gpio_t wifi_gpios[] = {
    {
        .gpio = S912_WIFI_SDIO_WAKE_HOST,
    },
    {
        // For debugging purposes.
        .gpio = S912_GPIODV(13),
    },
};

static const wifi_config_t wifi_config = {
    .oob_irq_mode = ZX_INTERRUPT_MODE_LEVEL_HIGH,
};

static const pbus_metadata_t wifi_metadata[] = {
    {
        .type = DEVICE_METADATA_PRIVATE,
        .data_buffer = &wifi_config,
        .data_size = sizeof(wifi_config),
    }};

static const pbus_dev_t sdio_children[] = {
    []() {
        // Wifi driver.
        pbus_dev_t dev;
        dev.name = "vim2-wifi";
        dev.gpio_list = wifi_gpios;
        dev.gpio_count = countof(wifi_gpios);
        dev.metadata_list = wifi_metadata;
        dev.metadata_count = countof(wifi_metadata);
        return dev;
    }(),
};

static const pbus_dev_t aml_sd_emmc_children[] = {
    []() {
        // Generic SDIO driver.
        pbus_dev_t dev;
        dev.name = "sdio";
        dev.child_list = sdio_children;
        dev.child_count = countof(sdio_children);
        return dev;
    }(),
};

static const pbus_mmio_t aml_sd_emmc_mmios[] = {
    {
        .base = 0xD0070000,
        .length = 0x2000,
    }};

static const pbus_irq_t aml_sd_emmc_irqs[] = {
    {
        .irq = 248,
        //c++ initialization error
        .mode = 0,
        //c++ initialization error
    },
};

static const pbus_bti_t aml_sd_emmc_btis[] = {
    {
        .iommu_index = 0,
        .bti_id = BTI_SDIO,
    },
};

static const pbus_gpio_t aml_sd_emmc_gpios[] = {
    {
        .gpio = S912_GPIOX(6),
    },
};

static aml_sd_emmc_config_t config = {
    .supports_dma = true,
    .min_freq = 400000,
    .max_freq = 125000000,
};

static const pbus_metadata_t aml_sd_emmc_metadata[] = {
    {
        .type = DEVICE_METADATA_PRIVATE,
        .data_buffer = &config,
        .data_size = sizeof(config),
    }};

static const pbus_dev_t aml_sd_emmc_dev = []() {
    pbus_dev_t dev;

    dev.name = "aml-sdio";
    dev.vid = PDEV_VID_AMLOGIC;
    dev.pid = PDEV_PID_GENERIC;
    dev.did = PDEV_DID_AMLOGIC_SD_EMMC;
    dev.mmio_list = aml_sd_emmc_mmios;
    dev.mmio_count = countof(aml_sd_emmc_mmios);
    dev.irq_list = aml_sd_emmc_irqs;
    dev.irq_count = countof(aml_sd_emmc_irqs);
    dev.gpio_list = aml_sd_emmc_gpios;
    dev.gpio_count = countof(aml_sd_emmc_gpios);
    dev.bti_list = aml_sd_emmc_btis;
    dev.bti_count = countof(aml_sd_emmc_btis);
    dev.metadata_list = aml_sd_emmc_metadata;
    dev.metadata_count = countof(aml_sd_emmc_metadata);
    dev.child_list = aml_sd_emmc_children;
    dev.child_count = countof(aml_sd_emmc_children);
    return dev;
}();

zx_status_t Vim::SdioInit() {
    zx_status_t status;

    gpio_impl_.SetAltFunction(S912_WIFI_SDIO_D0, S912_WIFI_SDIO_D0_FN);
    gpio_impl_.SetAltFunction(S912_WIFI_SDIO_D1, S912_WIFI_SDIO_D1_FN);
    gpio_impl_.SetAltFunction(S912_WIFI_SDIO_D2, S912_WIFI_SDIO_D2_FN);
    gpio_impl_.SetAltFunction(S912_WIFI_SDIO_D3, S912_WIFI_SDIO_D3_FN);
    gpio_impl_.SetAltFunction(S912_WIFI_SDIO_CLK, S912_WIFI_SDIO_CLK_FN);
    gpio_impl_.SetAltFunction(S912_WIFI_SDIO_CMD, S912_WIFI_SDIO_CMD_FN);
    gpio_impl_.SetAltFunction(S912_WIFI_SDIO_WAKE_HOST, S912_WIFI_SDIO_WAKE_HOST_FN);

    if ((status = pbus_.DeviceAdd(&aml_sd_emmc_dev)) != ZX_OK) {
        zxlogf(ERROR, "SdioInit could not add aml_sd_emmc_dev: %d\n", status);
        return status;
    }

    return ZX_OK;
}
} //namespace vim
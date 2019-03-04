// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "aml-mipi-regs.h"
#include "aml-mipi.h"
#include <ddk/debug.h>
#include <hw/reg.h>
#include <stdint.h>
#include <zircon/types.h>

namespace camera {

void AmlMipiDevice::DumpCsiPhyRegs() {
    zxlogf(INFO, "%s: MIPI CSI PHY REGS VALUES\n", __FUNCTION__);
    for (int i = 0; i < 27; i++) {
        zxlogf(INFO, "0x%x  ---> 0x%x \n", (0xFF650000 + i * 4), csi_phy0_mmio_->Read32(i * 4));
    }
}

void AmlMipiDevice::DumpAPhyRegs() {
    zxlogf(INFO, "%s: MIPI APHY REGS VALUES\n", __FUNCTION__);
    for (int i = 0x13; i < 0x17; i++) {
        zxlogf(INFO, "0x%x  ---> 0x%x \n", (0xFF63c300 + i * 4), aphy0_mmio_->Read32(i * 4));
    }
}

void AmlMipiDevice::DumpHostRegs() {
    zxlogf(INFO, "%s: MIPI HOST REGS VALUES\n", __FUNCTION__);
    for (int i = 0; i < 8; i++) {
        zxlogf(INFO, "0x%x  ---> 0x%x \n", (0xff654000 + i * 4), csi_host0_mmio_->Read32(i * 4));
    }
}

void AmlMipiDevice::DumpFrontEndRegs() {
    zxlogf(INFO, "%s: MIPI ADAP FRONTEND REGS VALUES\n", __FUNCTION__);
    auto frontend_reg = mipi_adap_mmio_->View(FRONTEND_BASE, 0x400);
    for (int i = 0; i < 21; i++) {
        printf("0x%x  ---> 0x%x \n", (0xFF654800 + i * 4), frontend_reg.Read32(i * 4));
    }
}

void AmlMipiDevice::DumpReaderRegs() {
    zxlogf(INFO, "%s: MIPI ADAP READER REGS VALUES\n", __FUNCTION__);
    auto reader_reg = mipi_adap_mmio_->View(RD_BASE, 0x100);
    for (int i = 0; i < 8; i++) {
        printf("0x%x  ---> 0x%x \n", (0xFF655000 + i * 4), reader_reg.Read32(i * 4));
    }
}

void AmlMipiDevice::DumpAlignRegs() {
    zxlogf(INFO, "%s: MIPI ADAP ALIGN REGS VALUES\n", __FUNCTION__);
    auto align_reg = mipi_adap_mmio_->View(ALIGN_BASE, 0x100);
    for (int i = 48; i < 58; i++) {
        printf("0x%x  ---> 0x%x \n", (0xFF655000 + i * 4), align_reg.Read32(i * 4));
    }
}

void AmlMipiDevice::DumpPixelRegs() {
    zxlogf(INFO, "%s: MIPI ADAP PIXEL REGS VALUES\n", __FUNCTION__);
    auto pixel_reg = mipi_adap_mmio_->View(PIXEL_BASE, 0x100);
    for (int i = 0x20; i < 0x24; i++) {
        printf("0x%x  ---> 0x%x \n", (0xFF655000 + i * 4), pixel_reg.Read32(i * 4));
    }
}

void AmlMipiDevice::DumpMiscRegs() {
    zxlogf(INFO, "%s: MIPI ADAP MISC REGS VALUES\n", __FUNCTION__);
    auto misc_reg = mipi_adap_mmio_->View(MISC_BASE, 0x100);
    for (int i = 0x40; i < 0x64; i++) {
        printf("0x%x  ---> 0x%x \n", (0xFF655000 + i * 4), misc_reg.Read32(i * 4));
    }
}

} // namespace camera

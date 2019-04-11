// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <atomic>
#include <ddk/metadata/camera.h>
#include <ddk/platform-defs.h>
#include <ddk/protocol/platform-device-lib.h>
#include <ddk/protocol/platform/bus.h>
#include <ddk/protocol/platform/device.h>
#include <ddktl/device.h>
#include <ddktl/mmio.h>
#include <ddktl/pdev.h>
#include <ddktl/protocol/ispimpl.h>
#include <ddktl/protocol/mipicsi.h>
#include <fbl/unique_ptr.h>

#include <lib/fzl/pinned-vmo.h>
#include <lib/zx/interrupt.h>
#include <threads.h>

namespace camera {
// |AmlMipiDevice| is spawned by the driver in |aml-mipi.cpp|
// to which the IMX 277 Sensor driver binds to.
// This class provides the ZX_PROTOCOL_MIPICSI ops for all of it's
// children.
class AmlMipiDevice;
using DeviceType = ddk::Device<AmlMipiDevice, ddk::Unbindable, ddk::GetProtocolable>;

class AmlMipiDevice : public DeviceType,
                      public ddk::MipiCsiProtocol<AmlMipiDevice, ddk::base_protocol> {
public:
    DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(AmlMipiDevice);

    explicit AmlMipiDevice(zx_device_t* parent)
        : DeviceType(parent), pdev_(parent), parent_protocol_(parent) {
        mipi_csi_protocol_t self{&mipi_csi_protocol_ops_, this};
        self_protocol_ = ddk::MipiCsiProtocolClient(&self);
    }

    static zx_status_t Create(zx_device_t* parent);

    ~AmlMipiDevice();

    // Methods required by the ddk.
    void DdkRelease();
    void DdkUnbind();
    zx_status_t DdkGetProtocol(uint32_t proto_id, void* out_protocol);

    // Methods for ZX_PROTOCOL_MIPI_CSI.
    zx_status_t MipiCsiInit(const mipi_info_t* mipi_info,
                            const mipi_adap_info_t* adap_info);
    zx_status_t MipiCsiDeInit();

private:
    zx_status_t InitBuffer(const mipi_adap_info_t* info, size_t size);
    zx_status_t InitPdev(zx_device_t* parent);
    zx_status_t Bind(camera_sensor_t* sensor_info);
    uint32_t AdapGetDepth(const mipi_adap_info_t* info);
    int AdapterIrqHandler();

    // MIPI Iternal APIs
    void MipiPhyInit(const mipi_info_t* info);
    void MipiCsi2Init(const mipi_info_t* info);
    void MipiPhyReset();
    void MipiCsi2Reset();

    // Mipi Adapter APIs
    zx_status_t MipiAdapInit(const mipi_adap_info_t* info);
    void MipiAdapStart(const mipi_adap_info_t* info);
    void MipiAdapReset();
    void MipiAdapStart();

    // Initialize the MIPI host to 720p by default.
    zx_status_t AdapFrontendInit(const mipi_adap_info_t* info);
    zx_status_t AdapReaderInit(const mipi_adap_info_t* info);
    zx_status_t AdapPixelInit(const mipi_adap_info_t* info);
    zx_status_t AdapAlignInit(const mipi_adap_info_t* info);

    // Set it up for the correct format and resolution.
    void AdapAlignStart(const mipi_adap_info_t* info);
    void AdapPixelStart(const mipi_adap_info_t* info);
    void AdapReaderStart(const mipi_adap_info_t* info);
    void AdapFrontEndStart(const mipi_adap_info_t* info);

    // MIPI clock.
    void InitMipiClock();

    // Debug.
    void DumpCsiPhyRegs();
    void DumpAPhyRegs();
    void DumpHostRegs();
    void DumpFrontEndRegs();
    void DumpReaderRegs();
    void DumpAlignRegs();
    void DumpPixelRegs();
    void DumpMiscRegs();

    std::optional<ddk::MmioBuffer> csi_phy0_mmio_;
    std::optional<ddk::MmioBuffer> aphy0_mmio_;
    std::optional<ddk::MmioBuffer> csi_host0_mmio_;
    std::optional<ddk::MmioBuffer> mipi_adap_mmio_;
    std::optional<ddk::MmioBuffer> hiu_mmio_;

    ddk::PDev pdev_;
    ddk::MipiCsiProtocolClient self_protocol_;
    ddk::IspImplProtocolClient parent_protocol_;

    zx::bti bti_;
    zx::interrupt adap_irq_;
    std::atomic<bool> running_;
    thrd_t irq_thread_;

    zx::vmo ring_buffer_vmo_;
    fzl::PinnedVmo pinned_ring_buffer_;
};

} // namespace camera

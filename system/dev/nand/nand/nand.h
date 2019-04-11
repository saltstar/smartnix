// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <threads.h>

#include <ddk/driver.h>
#include <ddktl/device.h>
#include <ddktl/protocol/nand.h>
#include <ddktl/protocol/rawnand.h>
#include <fbl/intrusive_double_list.h>
#include <fbl/mutex.h>
#include <lib/operation/nand.h>
#include <lib/fzl/vmo-mapper.h>
#include <lib/zx/event.h>
#include <zircon/thread_annotations.h>
#include <zircon/types.h>

namespace nand {

using Transaction = nand::UnownedOperation<>;

class NandDevice;
using DeviceType = ddk::Device<NandDevice, ddk::GetSizable, ddk::Unbindable>;

class NandDevice : public DeviceType,
                   public ddk::NandProtocol<NandDevice, ddk::base_protocol> {
public:
    explicit NandDevice(zx_device_t* parent)
        : DeviceType(parent), raw_nand_(parent) {}

    DISALLOW_COPY_ASSIGN_AND_MOVE(NandDevice);

    ~NandDevice();

    static zx_status_t Create(void* ctx, zx_device_t* parent);
    zx_status_t Bind();
    zx_status_t Init();

    // Device protocol implementation.
    zx_off_t DdkGetSize() {
        return device_get_size(parent());
    }
    void DdkUnbind() { DdkRemove(); }
    void DdkRelease();

    // Nand protocol implementation.
    void NandQuery(fuchsia_hardware_nand_Info* info_out, size_t* nand_op_size_out);
    void NandQueue(nand_operation_t* op, nand_queue_callback completion_cb, void* cookie);
    zx_status_t NandGetFactoryBadBlockList(uint32_t* bad_blocks, size_t bad_block_len,
                                           size_t* num_bad_blocks);

private:
    // Maps the data and oob vmos from the specified |nand_op| into memory.
    zx_status_t MapVmos(const nand_operation_t& nand_op, fzl::VmoMapper* data, uint8_t** vaddr_data,
                        fzl::VmoMapper* oob, uint8_t** vaddr_oob);

    // Calls controller specific read function.
    // data, oob: pointers to user oob/data buffers.
    // nand_page : NAND page address to read.
    // ecc_correct : Number of ecc corrected bitflips (< 0 indicates
    // ecc could not correct all bitflips - caller needs to check that).
    // retries : Retry logic may not be needed.
    zx_status_t ReadPage(void* data, void* oob, uint32_t nand_page, uint32_t* corrected_bits,
                         size_t retries);

    zx_status_t EraseOp(nand_operation_t* nand_op);
    zx_status_t ReadOp(nand_operation_t* nand_op);
    zx_status_t WriteOp(nand_operation_t* nand_op);

    void DoIo(Transaction txn);
    zx_status_t WorkerThread();

    ddk::RawNandProtocolClient raw_nand_;

    fuchsia_hardware_nand_Info nand_info_;
    uint32_t num_nand_pages_;

    nand::UnownedOperationQueue<> txn_queue_;

    thrd_t worker_thread_;
    zx::event worker_event_;
};

} // namespace nand

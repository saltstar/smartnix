// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef ZIRCON_SYSTEM_DEV_BUS_VIRTIO_BLOCK_H_
#define ZIRCON_SYSTEM_DEV_BUS_VIRTIO_BLOCK_H_

#include "device.h"
#include "ring.h"

#include <atomic>
#include <stdlib.h>
#include <zircon/compiler.h>

#include "backends/backend.h"
#include <ddk/protocol/block.h>
#include <virtio/block.h>
#include <zircon/device/block.h>

#include <lib/sync/completion.h>

namespace virtio {

struct block_txn_t {
    block_op_t op;
    block_impl_queue_callback completion_cb;
    void* cookie;
    struct vring_desc* desc;
    size_t index;
    list_node_t node;
    zx_handle_t pmt;
};

class Ring;

class BlockDevice : public Device {
public:
    BlockDevice(zx_device_t* device, zx::bti bti, fbl::unique_ptr<Backend> backend);

    virtual zx_status_t Init() override;
    virtual void Release() override;
    virtual void Unbind() override;

    virtual void IrqRingUpdate() override;
    virtual void IrqConfigChange() override;

    uint64_t GetSize() const { return config_.capacity * config_.blk_size; }
    uint32_t GetBlockSize() const { return config_.blk_size; }
    uint64_t GetBlockCount() const { return config_.capacity; }
    const char* tag() const override { return "virtio-blk"; }

private:
    // DDK driver hooks
    static zx_off_t virtio_block_get_size(void* ctx);

    static void virtio_block_query(void* ctx, block_info_t* bi, size_t* bopsz);
    static void virtio_block_queue(void* ctx, block_op_t* bop,
                                   block_impl_queue_callback completion_cb, void* cookie);

    static void virtio_block_unbind(void* ctx);
    static void virtio_block_release(void* ctx);

    void GetInfo(block_info_t* info);

    void SignalWorker(block_txn_t* txn);
    void WorkerThread();
    void FlushPendingTxns();
    void CleanupPendingTxns();

    zx_status_t QueueTxn(block_txn_t* txn, uint32_t type, size_t bytes, uint64_t* pages,
                         size_t pagecount, uint16_t* idx);

    void txn_complete(block_txn_t* txn, zx_status_t status);

    // The main virtio ring.
    Ring vring_ = {this};

    // Lock to be used around Ring::AllocDescChain and FreeDesc.
    // TODO: Move this into Ring class once it's certain that other users of the class are okay with
    // it.
    fbl::Mutex ring_lock_;

    static const uint16_t ring_size = 128; // 128 matches legacy pci.

    // Saved block device configuration out of the pci config BAR.
    virtio_blk_config_t config_ = {};

    // A queue of block request/responses.
    static const size_t blk_req_count = 32;

    io_buffer_t blk_req_buf_;
    virtio_blk_req_t* blk_req_ = nullptr;

    zx_paddr_t blk_res_pa_ = 0;
    uint8_t* blk_res_ = nullptr;

    uint32_t blk_req_bitmap_ = 0;
    static_assert(blk_req_count <= sizeof(blk_req_bitmap_) * CHAR_BIT, "");

    size_t alloc_blk_req() {
        size_t i = 0;
        if (blk_req_bitmap_ != 0)
            i = sizeof(blk_req_bitmap_) * CHAR_BIT - __builtin_clz(blk_req_bitmap_);
        blk_req_bitmap_ |= (1 << i);
        return i;
    }

    void free_blk_req(size_t i) { blk_req_bitmap_ &= ~(1 << i); }

    // Pending txns and completion signal.
    fbl::Mutex txn_lock_;
    list_node pending_txn_list_ = LIST_INITIAL_VALUE(pending_txn_list_);
    sync_completion_t txn_signal_;

    // Worker state.
    thrd_t worker_thread_;
    list_node worker_txn_list_ = LIST_INITIAL_VALUE(worker_txn_list_);
    sync_completion_t worker_signal_;
    std::atomic_bool worker_shutdown_ = false;

    block_impl_protocol_ops_t block_ops_ = {};
};

} // namespace virtio

#endif // ZIRCON_SYSTEM_DEV_BUS_VIRTIO_BLOCK_H_

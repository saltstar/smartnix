// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <atomic>
#include <limits>
#include <new>
#include <string.h>
#include <threads.h>
#include <unistd.h>
#include <utility>

#include <ddk/protocol/block.h>
#include <fbl/array.h>
#include <fbl/auto_call.h>
#include <fbl/auto_lock.h>
#include <lib/fzl/owned-vmo-mapper.h>
#include <lib/sync/completion.h>
#include <lib/zx/vmo.h>
#include <zircon/compiler.h>
#include <zircon/device/block.h>
#include <zircon/syscalls.h>
#include <zircon/thread_annotations.h>

#include "fvm-private.h"
#include "slice-extent.h"
#include "vpartition.h"

namespace fvm {
namespace {

zx_status_t FvmLoadThread(void* arg) {
    return reinterpret_cast<fvm::VPartitionManager*>(arg)->Load();
}

} // namespace

VPartitionManager::VPartitionManager(zx_device_t* parent, const block_info_t& info,
                                     size_t block_op_size, const block_impl_protocol_t* bp)
    : ManagerDeviceType(parent), info_(info), metadata_size_(0), slice_size_(0),
      pslice_total_count_(0), pslice_allocated_count_(0), block_op_size_(block_op_size) {
    memcpy(&bp_, bp, sizeof(*bp));
}

VPartitionManager::~VPartitionManager() = default;

// static
zx_status_t VPartitionManager::Bind(zx_device_t* dev) {
    block_info_t block_info;
    block_impl_protocol_t bp;
    size_t block_op_size = 0;
    if (device_get_protocol(dev, ZX_PROTOCOL_BLOCK, &bp) != ZX_OK) {
        printf("fvm: ERROR: block device '%s': does not support block protocol\n",
               device_get_name(dev));
        return ZX_ERR_NOT_SUPPORTED;
    }
    bp.ops->query(bp.ctx, &block_info, &block_op_size);

    fbl::AllocChecker ac;
    auto vpm =
        fbl::make_unique_checked<VPartitionManager>(&ac, dev, block_info, block_op_size, &bp);
    if (!ac.check()) {
        return ZX_ERR_NO_MEMORY;
    }

    zx_status_t status = vpm->DdkAdd("fvm", DEVICE_ADD_INVISIBLE);
    if (status != ZX_OK) {
        return status;
    }

    // Read vpartition table asynchronously.
    int rc =
        thrd_create_with_name(&vpm->initialization_thread_, FvmLoadThread, vpm.get(), "fvm-init");
    if (rc < 0) {
        // See comment in Load()
        if (!vpm->device_remove_.exchange(true)) {
            vpm->DdkRemove();
        }
        return ZX_ERR_NO_MEMORY;
    }

    // The VPartitionManager object is owned by the DDK, now that it has been
    // added. It will be deleted when the device is released.
    __UNUSED auto ptr = vpm.release();
    return ZX_OK;
}

zx_status_t VPartitionManager::AddPartition(fbl::unique_ptr<VPartition> vp) const {
    auto ename = reinterpret_cast<const char*>(GetAllocatedVPartEntry(vp->GetEntryIndex())->name);
    char name[FVM_NAME_LEN + 32];
    snprintf(name, sizeof(name), "%.*s-p-%zu", FVM_NAME_LEN, ename, vp->GetEntryIndex());

    zx_status_t status;
    if ((status = vp->DdkAdd(name)) != ZX_OK) {
        return status;
    }
    // TODO(johngro): ask smklein why it is OK to release this managed pointer.
    __UNUSED auto ptr = vp.release();
    return ZX_OK;
}

struct VpmIoCookie {
    std::atomic<size_t> num_txns;
    std::atomic<zx_status_t> status;
    sync_completion_t signal;
};

static void IoCallback(void* cookie, zx_status_t status, block_op_t* op) {
    VpmIoCookie* c = reinterpret_cast<VpmIoCookie*>(cookie);
    if (status != ZX_OK) {
        c->status.store(status);
    }
    if (c->num_txns.fetch_sub(1) - 1 == 0) {
        sync_completion_signal(&c->signal);
    }
}

zx_status_t VPartitionManager::DoIoLocked(zx_handle_t vmo, size_t off, size_t len,
                                          uint32_t command) {
    const size_t block_size = info_.block_size;
    const size_t max_transfer = info_.max_transfer_size / block_size;
    size_t len_remaining = len / block_size;
    size_t vmo_offset = 0;
    size_t dev_offset = off / block_size;
    const size_t num_data_txns = fbl::round_up(len_remaining, max_transfer) / max_transfer;

    // Add a "FLUSH" operation to write requests.
    const bool flushing = command == BLOCK_OP_WRITE;
    const size_t num_txns = num_data_txns + (flushing ? 1 : 0);

    fbl::AllocChecker ac;
    fbl::Array<uint8_t> buffer(new (&ac) uint8_t[block_op_size_ * num_txns],
                               block_op_size_ * num_txns);

    if (!ac.check()) {
        return ZX_ERR_NO_MEMORY;
    }

    VpmIoCookie cookie;
    cookie.num_txns.store(num_txns);
    cookie.status.store(ZX_OK);
    sync_completion_reset(&cookie.signal);

    for (size_t i = 0; i < num_data_txns; i++) {
        size_t length = fbl::min(len_remaining, max_transfer);
        len_remaining -= length;

        block_op_t* bop = reinterpret_cast<block_op_t*>(buffer.get() + (block_op_size_ * i));

        bop->command = command;
        bop->rw.vmo = vmo;
        bop->rw.length = static_cast<uint32_t>(length);
        bop->rw.offset_dev = dev_offset;
        bop->rw.offset_vmo = vmo_offset;
        memset(buffer.get() + (block_op_size_ * i) + sizeof(block_op_t), 0,
               block_op_size_ - sizeof(block_op_t));
        vmo_offset += length;
        dev_offset += length;

        Queue(bop, IoCallback, &cookie);
    }

    if (flushing) {
        block_op_t* bop =
            reinterpret_cast<block_op_t*>(buffer.get() + (block_op_size_ * num_data_txns));
        memset(bop, 0, sizeof(*bop));
        bop->command = BLOCKIO_FLUSH;
        Queue(bop, IoCallback, &cookie);
    }

    ZX_DEBUG_ASSERT(len_remaining == 0);
    sync_completion_wait(&cookie.signal, ZX_TIME_INFINITE);
    return static_cast<zx_status_t>(cookie.status.load());
}

zx_status_t VPartitionManager::Load() {
    fbl::AutoLock lock(&lock_);

    auto auto_detach = fbl::MakeAutoCall([&]() TA_NO_THREAD_SAFETY_ANALYSIS {
        // Need to release the lock before calling DdkRemove(), since it will
        // free |this|.  Need to disable thread safety analysis since it doesn't
        // recognize that we were holding lock_.
        lock.release();

        fprintf(stderr, "fvm: Aborting Driver Load\n");
        // DdkRemove will cause the Release() hook to be called, cleaning up our
        // state.  The exchange below is sufficient to protect against a
        // use-after-free, since if DdkRemove() has already been called by
        // another thread (via DdkUnbind()), the release hook will block on thread_join()
        // until this method returns.
        if (!device_remove_.exchange(true)) {
            DdkRemove();
        }
    });

    zx::vmo vmo;
    if (zx::vmo::create(FVM_BLOCK_SIZE, 0, &vmo) != ZX_OK) {
        return ZX_ERR_INTERNAL;
    }

    // Read the superblock first, to determine the slice sice
    if (DoIoLocked(vmo.get(), 0, FVM_BLOCK_SIZE, BLOCK_OP_READ)) {
        fprintf(stderr, "fvm: Failed to read first block from underlying device\n");
        return ZX_ERR_INTERNAL;
    }

    fvm_t sb;
    zx_status_t status = vmo.read(&sb, 0, sizeof(sb));
    if (status != ZX_OK) {
        return ZX_ERR_INTERNAL;
    }

    // Validate the superblock, confirm the slice size
    slice_size_ = sb.slice_size;
    if ((slice_size_ * VSliceMax()) / VSliceMax() != slice_size_) {
        fprintf(stderr, "fvm: Slice Size, VSliceMax overflow block address space\n");
        return ZX_ERR_BAD_STATE;
    } else if (info_.block_size == 0 || SliceSize() % info_.block_size) {
        fprintf(stderr, "fvm: Bad block (%u) or slice size (%zu)\n", info_.block_size, SliceSize());
        return ZX_ERR_BAD_STATE;
    } else if (sb.vpartition_table_size != kVPartTableLength) {
        fprintf(stderr, "fvm: Bad vpartition table size %zu (expected %zu)\n",
                sb.vpartition_table_size, kVPartTableLength);
        return ZX_ERR_BAD_STATE;
    } else if (sb.allocation_table_size != AllocTableLength(DiskSize(), SliceSize())) {
        fprintf(stderr, "fvm: Bad allocation table size %zu (expected %zu)\n",
                sb.allocation_table_size, AllocTableLength(DiskSize(), SliceSize()));
        return ZX_ERR_BAD_STATE;
    }

    // Cache calculated FVM information.
    metadata_size_ = fvm::MetadataSize(DiskSize(), SliceSize());
    pslice_total_count_ = UsableSlicesCount(DiskSize(), SliceSize());

    // Now that the slice size is known, read the rest of the metadata
    auto make_metadata_vmo = [&](size_t offset, fzl::OwnedVmoMapper* out_mapping) {
        fzl::OwnedVmoMapper mapper;
        zx_status_t status = mapper.CreateAndMap(MetadataSize(), "fvm-metadata");
        if (status != ZX_OK) {
            return status;
        }

        // Read both copies of metadata, ensure at least one is valid
        if ((status = DoIoLocked(mapper.vmo().get(), offset, MetadataSize(), BLOCK_OP_READ)) !=
            ZX_OK) {
            return status;
        }

        *out_mapping = std::move(mapper);
        return ZX_OK;
    };

    fzl::OwnedVmoMapper mapper;
    if ((status = make_metadata_vmo(0, &mapper)) != ZX_OK) {
        fprintf(stderr, "fvm: Failed to load metadata vmo: %d\n", status);
        return status;
    }
    fzl::OwnedVmoMapper mapper_backup;
    if ((status = make_metadata_vmo(MetadataSize(), &mapper_backup)) != ZX_OK) {
        fprintf(stderr, "fvm: Failed to load backup metadata vmo: %d\n", status);
        return status;
    }

    const void* metadata;
    if ((status = fvm_validate_header(mapper.start(), mapper_backup.start(), MetadataSize(),
                                      &metadata)) != ZX_OK) {
        fprintf(stderr, "fvm: Header validation failure: %d\n", status);
        return status;
    }

    if (metadata == mapper.start()) {
        first_metadata_is_primary_ = true;
        metadata_ = std::move(mapper);
    } else {
        first_metadata_is_primary_ = false;
        metadata_ = std::move(mapper_backup);
    }

    // Begin initializing the underlying partitions
    DdkMakeVisible();
    auto_detach.cancel();

    // 0th vpartition is invalid
    fbl::unique_ptr<VPartition> vpartitions[FVM_MAX_ENTRIES] = {};

    // Iterate through FVM Entry table, allocating the VPartitions which
    // claim to have slices.
    for (size_t i = 1; i < FVM_MAX_ENTRIES; i++) {
        if (GetVPartEntryLocked(i)->slices == 0) {
            continue;
        } else if ((status = VPartition::Create(this, i, &vpartitions[i])) != ZX_OK) {
            fprintf(stderr, "FVM: Failed to Create vpartition %zu\n", i);
            return status;
        }
    }

    // Iterate through the Slice Allocation table, filling the slice maps
    // of VPartitions.
    for (uint32_t i = 1; i <= GetFvmLocked()->pslice_count; i++) {
        const slice_entry_t* entry = GetSliceEntryLocked(i);
        if (entry->Vpart() == FVM_SLICE_ENTRY_FREE) {
            continue;
        }
        if (vpartitions[entry->Vpart()] == nullptr) {
            continue;
        }

        // It's fine to load the slices while not holding the vpartition
        // lock; no VPartition devices exist yet.
        vpartitions[entry->Vpart()]->SliceSetUnsafe(entry->Vslice(), i);
        pslice_allocated_count_++;
    }

    lock.release();

    // Iterate through 'valid' VPartitions, and create their devices.
    size_t device_count = 0;
    for (size_t i = 0; i < FVM_MAX_ENTRIES; i++) {
        if (vpartitions[i] == nullptr) {
            continue;
        } else if (GetAllocatedVPartEntry(i)->flags & kVPartFlagInactive) {
            fprintf(stderr, "FVM: Freeing inactive partition\n");
            FreeSlices(vpartitions[i].get(), 0, VSliceMax());
            continue;
        } else if (AddPartition(std::move(vpartitions[i]))) {
            continue;
        }
        device_count++;
    }

    return ZX_OK;
}

zx_status_t VPartitionManager::WriteFvmLocked() {
    zx_status_t status;

    GetFvmLocked()->generation++;
    fvm_update_hash(GetFvmLocked(), MetadataSize());

    // If we were reading from the primary, write to the backup.
    status =
        DoIoLocked(metadata_.vmo().get(), BackupOffsetLocked(), MetadataSize(), BLOCK_OP_WRITE);
    if (status != ZX_OK) {
        fprintf(stderr, "FVM: Failed to write metadata\n");
        return status;
    }

    // We only allow the switch of "write to the other copy of metadata"
    // once a valid version has been written entirely.
    first_metadata_is_primary_ = !first_metadata_is_primary_;
    return ZX_OK;
}

zx_status_t VPartitionManager::FindFreeVPartEntryLocked(size_t* out) const {
    for (size_t i = 1; i < FVM_MAX_ENTRIES; i++) {
        const vpart_entry_t* entry = GetVPartEntryLocked(i);
        if (entry->slices == 0) {
            *out = i;
            return ZX_OK;
        }
    }
    return ZX_ERR_NO_SPACE;
}

zx_status_t VPartitionManager::FindFreeSliceLocked(size_t* out, size_t hint) const {
    hint = fbl::max(hint, 1lu);
    for (size_t i = hint; i <= pslice_total_count_; i++) {
        if (GetSliceEntryLocked(i)->Vpart() == FVM_SLICE_ENTRY_FREE) {
            *out = i;
            return ZX_OK;
        }
    }
    for (size_t i = 1; i < hint; i++) {
        if (GetSliceEntryLocked(i)->Vpart() == FVM_SLICE_ENTRY_FREE) {
            *out = i;
            return ZX_OK;
        }
    }
    return ZX_ERR_NO_SPACE;
}

zx_status_t VPartitionManager::AllocateSlices(VPartition* vp, size_t vslice_start, size_t count) {
    fbl::AutoLock lock(&lock_);
    return AllocateSlicesLocked(vp, vslice_start, count);
}

zx_status_t VPartitionManager::AllocateSlicesLocked(VPartition* vp, size_t vslice_start,
                                                    size_t count) {
    if (vslice_start + count > VSliceMax()) {
        return ZX_ERR_INVALID_ARGS;
    }

    zx_status_t status = ZX_OK;
    size_t hint = 0;

    {
        fbl::AutoLock lock(&vp->lock_);
        if (vp->IsKilledLocked()) {
            return ZX_ERR_BAD_STATE;
        }
        for (size_t i = 0; i < count; i++) {
            size_t pslice;
            auto vslice = vslice_start + i;
            if (vp->SliceGetLocked(vslice) != PSLICE_UNALLOCATED) {
                status = ZX_ERR_INVALID_ARGS;
            }
            if ((status != ZX_OK) || ((status = FindFreeSliceLocked(&pslice, hint)) != ZX_OK) ||
                ((status = vp->SliceSetLocked(vslice, static_cast<uint32_t>(pslice)) != ZX_OK))) {
                for (int j = static_cast<int>(i - 1); j >= 0; j--) {
                    vslice = vslice_start + j;
                    FreePhysicalSlice(vp, vp->SliceGetLocked(vslice));
                    vp->SliceFreeLocked(vslice);
                }

                return status;
            }
            AllocatePhysicalSlice(vp, pslice, vslice);
            hint = pslice + 1;
        }
    }

    if ((status = WriteFvmLocked()) != ZX_OK) {
        // Undo allocation in the event of failure; avoid holding VPartition
        // lock while writing to fvm.
        fbl::AutoLock lock(&vp->lock_);
        for (int j = static_cast<int>(count - 1); j >= 0; j--) {
            auto vslice = vslice_start + j;
            FreePhysicalSlice(vp, vp->SliceGetLocked(vslice));
            vp->SliceFreeLocked(vslice);
        }
    }

    return status;
}

zx_status_t VPartitionManager::Upgrade(const uint8_t* old_guid, const uint8_t* new_guid) {
    fbl::AutoLock lock(&lock_);
    size_t old_index = 0;
    size_t new_index = 0;

    if (!memcmp(old_guid, new_guid, GUID_LEN)) {
        old_guid = nullptr;
    }

    for (size_t i = 1; i < FVM_MAX_ENTRIES; i++) {
        auto entry = GetVPartEntryLocked(i);
        if (entry->slices != 0) {
            if (old_guid && !(entry->flags & kVPartFlagInactive) &&
                !memcmp(entry->guid, old_guid, GUID_LEN)) {
                old_index = i;
            } else if ((entry->flags & kVPartFlagInactive) &&
                       !memcmp(entry->guid, new_guid, GUID_LEN)) {
                new_index = i;
            }
        }
    }

    if (!new_index) {
        return ZX_ERR_NOT_FOUND;
    }

    if (old_index) {
        GetVPartEntryLocked(old_index)->flags |= kVPartFlagInactive;
    }
    GetVPartEntryLocked(new_index)->flags &= ~kVPartFlagInactive;

    return WriteFvmLocked();
}

zx_status_t VPartitionManager::FreeSlices(VPartition* vp, size_t vslice_start, size_t count) {
    fbl::AutoLock lock(&lock_);
    return FreeSlicesLocked(vp, vslice_start, count);
}

zx_status_t VPartitionManager::FreeSlicesLocked(VPartition* vp, size_t vslice_start, size_t count) {
    if (vslice_start + count > VSliceMax() || count > VSliceMax()) {
        return ZX_ERR_INVALID_ARGS;
    }

    bool freed_something = false;
    {
        fbl::AutoLock lock(&vp->lock_);
        if (vp->IsKilledLocked())
            return ZX_ERR_BAD_STATE;

        if (vslice_start == 0) {
            // Special case: Freeing entire VPartition
            for (auto extent = vp->ExtentBegin(); extent.IsValid(); extent = vp->ExtentBegin()) {
                for (size_t i = extent->start(); i < extent->end(); i++) {
                    FreePhysicalSlice(vp, vp->SliceGetLocked(i));
                }
                vp->ExtentDestroyLocked(extent->start());
            }

            // Remove device, VPartition if this was a request to free all slices.
            vp->DdkRemove();
            auto entry = GetVPartEntryLocked(vp->GetEntryIndex());
            entry->clear();
            vp->KillLocked();
            freed_something = true;
        } else {
            for (int i = static_cast<int>(count - 1); i >= 0; i--) {
                auto vslice = vslice_start + i;
                if (vp->SliceCanFree(vslice)) {
                    size_t pslice = vp->SliceGetLocked(vslice);
                    if (!freed_something) {
                        // The first 'free' is the only one which can fail -- it
                        // has the potential to split extents, which may require
                        // memory allocation.
                        if (!vp->SliceFreeLocked(vslice)) {
                            return ZX_ERR_NO_MEMORY;
                        }
                    } else {
                        ZX_ASSERT(vp->SliceFreeLocked(vslice));
                    }
                    FreePhysicalSlice(vp, pslice);
                    freed_something = true;
                }
            }
        }
    }

    if (!freed_something) {
        return ZX_ERR_INVALID_ARGS;
    }
    return WriteFvmLocked();
}

void VPartitionManager::Query(fvm_info_t* info) {
    info->slice_size = SliceSize();
    info->vslice_count = VSliceMax();
    {
        fbl::AutoLock lock(&lock_);
        info->pslice_total_count = pslice_total_count_;
        info->pslice_allocated_count = pslice_allocated_count_;
    }
}

void VPartitionManager::FreePhysicalSlice(VPartition* vp, size_t pslice) {
    auto entry = GetSliceEntryLocked(pslice);
    ZX_DEBUG_ASSERT_MSG(entry->Vpart() != FVM_SLICE_ENTRY_FREE, "Freeing already-free slice");
    entry->SetVpart(FVM_SLICE_ENTRY_FREE);
    GetVPartEntryLocked(vp->GetEntryIndex())->slices--;
    pslice_allocated_count_--;
}

void VPartitionManager::AllocatePhysicalSlice(VPartition* vp, size_t pslice, uint64_t vslice) {
    uint64_t vpart = vp->GetEntryIndex();
    ZX_DEBUG_ASSERT(vpart <= VPART_MAX);
    ZX_DEBUG_ASSERT(vslice <= VSLICE_MAX);
    auto entry = GetSliceEntryLocked(pslice);
    ZX_DEBUG_ASSERT_MSG(entry->Vpart() == FVM_SLICE_ENTRY_FREE,
                        "Allocating previously allocated slice");
    entry->SetVpart(vpart);
    entry->SetVslice(vslice);
    GetVPartEntryLocked(vpart)->slices++;
    pslice_allocated_count_++;
}

slice_entry_t* VPartitionManager::GetSliceEntryLocked(size_t index) const {
    ZX_DEBUG_ASSERT(index >= 1);
    uintptr_t metadata_start = reinterpret_cast<uintptr_t>(GetFvmLocked());
    uintptr_t offset = static_cast<uintptr_t>(kAllocTableOffset + index * sizeof(slice_entry_t));
    ZX_DEBUG_ASSERT(kAllocTableOffset <= offset);
    ZX_DEBUG_ASSERT(offset < kAllocTableOffset + AllocTableLength(DiskSize(), SliceSize()));
    return reinterpret_cast<slice_entry_t*>(metadata_start + offset);
}

vpart_entry_t* VPartitionManager::GetVPartEntryLocked(size_t index) const {
    ZX_DEBUG_ASSERT(index >= 1);
    uintptr_t metadata_start = reinterpret_cast<uintptr_t>(GetFvmLocked());
    uintptr_t offset = static_cast<uintptr_t>(kVPartTableOffset + index * sizeof(vpart_entry_t));
    ZX_DEBUG_ASSERT(kVPartTableOffset <= offset);
    ZX_DEBUG_ASSERT(offset < kVPartTableOffset + kVPartTableLength);
    return reinterpret_cast<vpart_entry_t*>(metadata_start + offset);
}

// Device protocol (FVM)

zx_status_t VPartitionManager::DdkIoctl(uint32_t op, const void* cmd, size_t cmdlen, void* reply,
                                        size_t max, size_t* out_actual) {
    switch (op) {
    case IOCTL_BLOCK_FVM_ALLOC_PARTITION: {
        if (cmdlen < sizeof(alloc_req_t))
            return ZX_ERR_BUFFER_TOO_SMALL;
        const alloc_req_t* request = static_cast<const alloc_req_t*>(cmd);

        if (request->slice_count >= std::numeric_limits<uint32_t>::max()) {
            return ZX_ERR_OUT_OF_RANGE;
        } else if (request->slice_count == 0) {
            return ZX_ERR_OUT_OF_RANGE;
        }

        zx_status_t status;
        fbl::unique_ptr<VPartition> vpart;
        {
            fbl::AutoLock lock(&lock_);
            size_t vpart_entry;
            if ((status = FindFreeVPartEntryLocked(&vpart_entry)) != ZX_OK) {
                return status;
            }

            if ((status = VPartition::Create(this, vpart_entry, &vpart)) != ZX_OK) {
                return status;
            }

            auto entry = GetVPartEntryLocked(vpart_entry);
            entry->init(request->type, request->guid, 0, request->name,
                        request->flags & kVPartAllocateMask);

            if ((status = AllocateSlicesLocked(vpart.get(), 0, request->slice_count)) != ZX_OK) {
                entry->slices = 0; // Undo VPartition allocation
                return status;
            }
        }
        if ((status = AddPartition(std::move(vpart))) != ZX_OK) {
            return status;
        }
        return ZX_OK;
    }
    case IOCTL_BLOCK_FVM_QUERY: {
        if (max < sizeof(fvm_info_t)) {
            return ZX_ERR_BUFFER_TOO_SMALL;
        }
        fvm_info_t* info = static_cast<fvm_info_t*>(reply);
        Query(info);
        *out_actual = sizeof(fvm_info_t);
        return ZX_OK;
    }
    case IOCTL_BLOCK_FVM_UPGRADE: {
        if (cmdlen < sizeof(upgrade_req_t)) {
            return ZX_ERR_BUFFER_TOO_SMALL;
        }
        const upgrade_req_t* req = static_cast<const upgrade_req_t*>(cmd);
        return Upgrade(req->old_guid, req->new_guid);
    }
    default:
        return ZX_ERR_NOT_SUPPORTED;
    }

    return ZX_ERR_NOT_SUPPORTED;
}

void VPartitionManager::DdkUnbind() {
    if (!device_remove_.exchange(true)) {
        DdkRemove();
    }
}

void VPartitionManager::DdkRelease() {
    thrd_join(initialization_thread_, nullptr);
    delete this;
}

} // namespace fvm

// C-compatibility definitions

zx_status_t fvm_bind(zx_device_t* parent) {
    return fvm::VPartitionManager::Bind(parent);
}

// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <blobfs/writeback.h>

#include <utility>

namespace blobfs {

WriteTxn::~WriteTxn() {
    ZX_DEBUG_ASSERT_MSG(requests_.is_empty(), "WriteTxn still has pending requests");
}

void WriteTxn::Enqueue(const zx::vmo& vmo, uint64_t relative_block, uint64_t absolute_block,
                       uint64_t nblocks) {
    ZX_DEBUG_ASSERT(vmo.is_valid());
    ZX_DEBUG_ASSERT(!IsBuffered());

    for (auto& request : requests_) {
        if (request.vmo != vmo.get()) {
            continue;
        }

        if (request.vmo_offset == relative_block) {
            // Take the longer of the operations (if operating on the same blocks).
            if (nblocks > request.length) {
                block_count_ += (nblocks - request.length);
                request.length = nblocks;
            }
            return;
        } else if ((request.vmo_offset + request.length == relative_block) &&
                   (request.dev_offset + request.length == absolute_block)) {
            // Combine with the previous request, if immediately following.
            request.length += nblocks;
            block_count_ += nblocks;
            return;
        }
    }

    WriteRequest request;
    request.vmo = vmo.get();
    request.vmo_offset = relative_block;
    request.dev_offset = absolute_block;
    request.length = nblocks;
    requests_.push_back(std::move(request));
    block_count_ += request.length;
}

size_t WriteTxn::BlkStart() const {
    ZX_DEBUG_ASSERT(IsBuffered());
    ZX_DEBUG_ASSERT(requests_.size() > 0);
    return requests_[0].vmo_offset;
}

size_t WriteTxn::BlkCount() const {
    return block_count_;
}

void WriteTxn::SetBuffer(vmoid_t vmoid) {
    ZX_DEBUG_ASSERT(vmoid_ == VMOID_INVALID || vmoid_ == vmoid);
    ZX_DEBUG_ASSERT(vmoid != VMOID_INVALID);
    vmoid_ = vmoid;
}

zx_status_t WriteTxn::Flush() {
    ZX_ASSERT(IsBuffered());
    fs::Ticker ticker(transaction_manager_->LocalMetrics().Collecting());

    // Update all the outgoing transactions to be in disk blocks
    block_fifo_request_t blk_reqs[requests_.size()];
    const uint32_t kDiskBlocksPerBlobfsBlock =
            kBlobfsBlockSize / transaction_manager_->DeviceBlockSize();
    for (size_t i = 0; i < requests_.size(); i++) {
        blk_reqs[i].group = transaction_manager_->BlockGroupID();
        blk_reqs[i].vmoid = vmoid_;
        blk_reqs[i].opcode = BLOCKIO_WRITE;
        blk_reqs[i].vmo_offset = requests_[i].vmo_offset * kDiskBlocksPerBlobfsBlock;
        blk_reqs[i].dev_offset = requests_[i].dev_offset * kDiskBlocksPerBlobfsBlock;
        uint64_t length = requests_[i].length * kDiskBlocksPerBlobfsBlock;
        // TODO(ZX-2253): Requests this long, although unlikely, should be
        // handled more gracefully.
        ZX_ASSERT_MSG(length < UINT32_MAX, "Request size too large");
        blk_reqs[i].length = static_cast<uint32_t>(length);
    }

    // Actually send the operations to the underlying block device.
    zx_status_t status = transaction_manager_->Transaction(blk_reqs, requests_.size());

    if (transaction_manager_->LocalMetrics().Collecting()) {
        uint64_t sum = 0;
        for (const auto& blk_req : blk_reqs) {
            sum += blk_req.length * kBlobfsBlockSize;
        }
        transaction_manager_->LocalMetrics().UpdateWriteback(sum, ticker.End());
    }

    requests_.reset();
    vmoid_ = VMOID_INVALID;
    block_count_ = 0;
    return status;
}

void WritebackWork::MarkCompleted(zx_status_t status) {
    WriteTxn::Reset();
    if (sync_cb_) {
        sync_cb_(status);
    }
    sync_cb_ = nullptr;
    ready_cb_ = nullptr;
}

bool WritebackWork::IsReady() {
    if (ready_cb_) {
        if (ready_cb_()) {
            ready_cb_ = nullptr;
            return true;
        }

        return false;
    }

    return true;
}

void WritebackWork::SetReadyCallback(ReadyCallback callback) {
    ZX_DEBUG_ASSERT(!ready_cb_);
    ready_cb_ = std::move(callback);
}

void WritebackWork::SetSyncCallback(SyncCallback callback) {
    if (sync_cb_) {
        // This "callback chain" allows multiple clients to observe the completion
        // of the WritebackWork. This is akin to a promise "and-then" relationship.
        sync_cb_ = [previous_callback = std::move(sync_cb_),
                    next_callback = std::move(callback)] (zx_status_t status) {
            next_callback(status);
            previous_callback(status);
        };
    } else {
        sync_cb_ = std::move(callback);
    }
}

// Returns the number of blocks of the writeback buffer that have been consumed
zx_status_t WritebackWork::Complete() {
    zx_status_t status = Flush();
    MarkCompleted(status);
    return status;
}

WritebackWork::WritebackWork(TransactionManager* transaction_manager)
    : WriteTxn(transaction_manager), ready_cb_(nullptr), sync_cb_(nullptr) {}

Buffer::~Buffer() {
    if (vmoid_ != VMOID_INVALID) {
        // Close the buffer vmo.
        block_fifo_request_t request;
        request.group = transaction_manager_->BlockGroupID();
        request.vmoid = vmoid_;
        request.opcode = BLOCKIO_CLOSE_VMO;
        transaction_manager_->Transaction(&request, 1);
    }
}

zx_status_t Buffer::Create(TransactionManager* blobfs, size_t blocks, const char* label,
                           fbl::unique_ptr<Buffer>* out) {

    fzl::OwnedVmoMapper mapper;
    zx_status_t status = mapper.CreateAndMap(blocks * kBlobfsBlockSize, "blob-writeback");
    if (status != ZX_OK) {
        FS_TRACE_ERROR("Buffer: Failed to create vmo\n");
        return status;
    }

    fbl::unique_ptr<Buffer> buffer(new Buffer(blobfs, std::move(mapper)));
    if ((status = buffer->transaction_manager_->AttachVmo(buffer->mapper_.vmo(), &buffer->vmoid_))
        != ZX_OK) {
        FS_TRACE_ERROR("Buffer: Failed to attach vmo\n");
        return status;
    }

    *out = std::move(buffer);
    return ZX_OK;
}

bool Buffer::IsSpaceAvailable(size_t blocks) const {
    // TODO(planders): Similar to minfs, make sure that we either have a fallback mechanism for
    // operations which are too large to be fully contained by the buffer, or that the
    // worst-case operation will always fit within the buffer.
    ZX_ASSERT_MSG(blocks <= capacity_, "Requested txn (%zu blocks) larger than buffer", blocks);
    return length_ + blocks <= capacity_;
}

void Buffer::CopyTransaction(WriteTxn* txn) {
    ZX_DEBUG_ASSERT(!txn->IsBuffered());
    auto& reqs = txn->Requests();

    for (size_t i = 0; i < reqs.size(); i++) {
        ZX_DEBUG_ASSERT(reqs[i].vmo != ZX_HANDLE_INVALID);

        // Read parameters of the current request.
        size_t vmo_offset = reqs[i].vmo_offset;
        size_t dev_offset = reqs[i].dev_offset;
        const size_t vmo_len = reqs[i].length;
        ZX_DEBUG_ASSERT(vmo_len > 0);

        // Calculate the offset/length we will need to write into the buffer.
        size_t buf_offset = (start_ + length_) % capacity_;
        size_t buf_len = (buf_offset + vmo_len > capacity_) ? capacity_ - buf_offset : vmo_len;
        size_t init_len = vmo_len;
        size_t total_len = buf_len;

        // Verify that the length is valid.
        ZX_DEBUG_ASSERT(buf_len > 0);
        ZX_DEBUG_ASSERT(buf_len <= vmo_len);
        ZX_DEBUG_ASSERT(buf_len < capacity_);
        zx_handle_t vmo = reqs[i].vmo;
        ZX_DEBUG_ASSERT(vmo != mapper_.vmo().get());

        // Write data from the vmo into the buffer.
        void* ptr = MutableData(buf_offset);

        zx_status_t status;
        ZX_DEBUG_ASSERT((start_ <= buf_offset) ?
                        (start_ < buf_offset + buf_len) :
                        (buf_offset + buf_len <= start_)); // Wraparound
        ZX_ASSERT_MSG((status = zx_vmo_read(vmo, ptr, vmo_offset * kBlobfsBlockSize,
                        buf_len * kBlobfsBlockSize)) == ZX_OK, "VMO Read Fail: %d", status);

        // Update the buffer len to include newly written data.
        length_ += buf_len;

        // Update the write_request to transfer from the writeback buffer out to disk,
        // rather than the supplied VMO.
        // Set the vmo handle to invalid, since we will be using the same vmoid for all requests.
        reqs[i].vmo = ZX_HANDLE_INVALID;
        reqs[i].vmo_offset = buf_offset;
        reqs[i].length = buf_len;

        if (buf_len != vmo_len) {
            // We wrapped around; write what remains from this request.
            vmo_offset += buf_len;
            dev_offset += buf_len;
            buf_len = vmo_len - buf_len;
            ZX_DEBUG_ASSERT(buf_len > 0);

            ptr = MutableData(0);
            ZX_DEBUG_ASSERT((start_ == 0) ? (start_ < buf_len) : (buf_len <= start_)); // Wraparound
            ZX_ASSERT(zx_vmo_read(vmo, ptr, vmo_offset * kBlobfsBlockSize,
                                  buf_len * kBlobfsBlockSize) == ZX_OK);

            length_ += buf_len;
            total_len += buf_len;

            // Shift down all following write requests.
            static_assert(fbl::is_pod<WriteRequest>::value, "Can't memmove non-POD");

            // Shift down all following write requests
            static_assert(fbl::is_pod<WriteRequest>::value, "Can't memmove non-POD");
            // Insert the "new" request, which is the latter half of the last request
            WriteRequest request;
            request.vmo = vmo;
            request.vmo_offset = 0;
            request.dev_offset = dev_offset;
            request.length = buf_len;
            i++;
            reqs.insert(i, request);
        }

        // Verify that the length of all vmo writes we did match the total length we were meant to
        // write from the initial vmo.
        ZX_DEBUG_ASSERT(init_len == total_len);
    }

    txn->SetBuffer(vmoid_);
}

void Buffer::AddTransaction(size_t start, size_t disk_start, size_t length, WritebackWork* work) {
    // Ensure the request fits within the buffer.
    ZX_DEBUG_ASSERT(length > 0);
    ZX_DEBUG_ASSERT(start + length <= capacity_);
    ZX_DEBUG_ASSERT(work != nullptr);
    work->Enqueue(mapper_.vmo(), start, disk_start, length);
}

bool Buffer::VerifyTransaction(WriteTxn* txn) const {
    if (txn->CheckBuffer(vmoid_)) {
        if (txn->BlkCount() > 0) {
            // If the work belongs to the WritebackQueue, verify that it matches up with the
            // buffer's start/len.
            ZX_ASSERT(txn->BlkStart() == start_);
            ZX_ASSERT(txn->BlkCount() <= length_);
        }

        return true;
    }

    return false;
}

void Buffer::ValidateTransaction(WriteTxn* txn) {
    if (txn->IsBuffered()) {
        // If transaction is already buffered, make sure it belongs to this buffer.
        ZX_DEBUG_ASSERT(txn->CheckBuffer(vmoid_));
    } else {
        fbl::Vector<WriteRequest>& reqs = txn->Requests();

        for (size_t i = 0; i < reqs.size(); i++) {
            // Verify that each request references this buffer VMO,
            // and that the transaction fits within the buffer.
            ZX_DEBUG_ASSERT(reqs[i].vmo == mapper_.vmo().get());
            reqs[i].vmo = ZX_HANDLE_INVALID;
        }

        // Once each request has been verified, set the buffer.
        txn->SetBuffer(vmoid_);
    }
}

void Buffer::FreeSpace(size_t blocks) {
    ZX_DEBUG_ASSERT(blocks <= length_);
    start_ = (start_ + blocks) % capacity_;
    length_ -= blocks;
}

WritebackQueue::~WritebackQueue() {
    // Ensure that thread teardown has completed, or that it was never brought up to begin with.
    ZX_DEBUG_ASSERT(!IsRunning());
    ZX_DEBUG_ASSERT(work_queue_.is_empty());
}

zx_status_t WritebackQueue::Teardown() {
    WritebackState state;

    {
        fbl::AutoLock lock(&lock_);
        state = state_;

        // Signal the background thread.
        unmounting_ = true;
        cnd_signal(&work_added_);
    }

    zx_status_t status = ZX_OK;
    if (state != WritebackState::kInit) {
        // Block until the thread completes itself.
        int result = -1;
        int success = thrd_join(worker_, &result);
        if (result != 0 || success != thrd_success) {
            status = ZX_ERR_INTERNAL;
        }
    }

    return status;
}

zx_status_t WritebackQueue::Create(TransactionManager* transaction_manager,
                                   const size_t buffer_blocks,
                                   fbl::unique_ptr<WritebackQueue>* out) {
    zx_status_t status;
    fbl::unique_ptr<Buffer> buffer;
    if ((status = Buffer::Create(transaction_manager, buffer_blocks, "blobfs-writeback", &buffer))
        != ZX_OK) {
        return status;
    }

    fbl::unique_ptr<WritebackQueue> wb(new WritebackQueue(std::move(buffer)));

    if (cnd_init(&wb->work_completed_) != thrd_success) {
        return ZX_ERR_NO_RESOURCES;
    }
    if (cnd_init(&wb->work_added_) != thrd_success) {
        return ZX_ERR_NO_RESOURCES;
    }

    if (thrd_create_with_name(&wb->worker_,
                              WritebackQueue::WritebackThread, wb.get(),
                              "blobfs-writeback") != thrd_success) {
        return ZX_ERR_NO_RESOURCES;
    }

    fbl::AutoLock lock(&wb->lock_);
    wb->state_ = WritebackState::kRunning;
    *out = std::move(wb);
    return ZX_OK;
}

zx_status_t WritebackQueue::Enqueue(fbl::unique_ptr<WritebackWork> work) {
    TRACE_DURATION("blobfs", "WritebackQueue::Enqueue", "work ptr", work.get());
    fbl::AutoLock lock(&lock_);
    zx_status_t status = ZX_OK;

    if (IsReadOnly()) {
        // If we are in a readonly state, return an error. However, the work should still be
        // enqueued and ultimately processed by the WritebackThread. This will help us avoid
        // potential race conditions if the work callback must acquire a lock.
        status = ZX_ERR_BAD_STATE;
    } else if (!work->IsBuffered()) {
        ZX_DEBUG_ASSERT(state_ == WritebackState::kRunning);

        // Only copy blocks to the buffer if they have not already been copied to another buffer.
        EnsureSpaceLocked(work->BlkCount());

        // It is possible that the queue entered a read only state
        // while we were waiting to ensure space, so check again now.
        if (IsReadOnly()) {
            status = ZX_ERR_BAD_STATE;
        } else {
            buffer_->CopyTransaction(work.get());
        }
    }

    work_queue_.push(std::move(work));
    cnd_signal(&work_added_);
    return status;
}

bool WritebackQueue::IsRunning() const {
    switch (state_) {
    case WritebackState::kRunning:
    case WritebackState::kReadOnly:
        return true;
    default:
        return false;
    }
}

void WritebackQueue::EnsureSpaceLocked(size_t blocks) {
    while (!buffer_->IsSpaceAvailable(blocks)) {
        // Not enough room to write back work, yet. Wait until room is available.
        Waiter w;
        producer_queue_.push(&w);

        do {
            cnd_wait(&work_completed_, lock_.GetInternal());
        } while ((&producer_queue_.front() != &w) && // We are first in line to enqueue...
                (!buffer_->IsSpaceAvailable(blocks))); // ... and there is enough space for us.

        producer_queue_.pop();
    }
}

int WritebackQueue::WritebackThread(void* arg) {
    WritebackQueue* b = reinterpret_cast<WritebackQueue*>(arg);

    b->lock_.Acquire();
    while (true) {
        bool error = b->IsReadOnly();
        while (!b->work_queue_.is_empty()) {
            if (!error && !b->work_queue_.front().IsReady()) {
                // If the work is not yet ready, break and wait until we receive another signal.
                break;
            }

            auto work = b->work_queue_.pop();
            TRACE_DURATION("blobfs", "WritebackQueue::WritebackThread", "work ptr", work.get());

            bool our_buffer = b->buffer_->VerifyTransaction(work.get());
            size_t blk_count = work->BlkCount();

            // Stay unlocked while processing a unit of work.
            b->lock_.Release();

            if (error) {
                // If we are in a read only state, mark the work complete with an error status.
                work->MarkCompleted(ZX_ERR_BAD_STATE);
            } else {
                // If we should complete the work, make sure it has been buffered.
                // (This is not necessary if we are currently in an error state).
                ZX_DEBUG_ASSERT(work->IsBuffered());
                zx_status_t status;
                if ((status = work->Complete()) != ZX_OK) {
                    FS_TRACE_ERROR("Work failed with status %d - "
                                   "converting writeback to read only state.\n", status);
                    // If work completion failed, set the buffer to an error state.
                    error = true;
                }
            }

            work = nullptr;
            b->lock_.Acquire();

            if (error) {
                // If we encountered an error, set the queue to readonly.
                b->state_ = WritebackState::kReadOnly;
            }

            if (our_buffer) {
                // If the last work we processed belonged to our buffer,
                // update the buffer's start/len accordingly.
                b->buffer_->FreeSpace(blk_count);
            }

            // We may have opened up space (or entered a read only state),
            // so signal the producer queue.
            cnd_signal(&b->work_completed_);
        }

        // Before waiting, we should check if we're unmounting.
        // If work still remains in the work or producer queues,
        // continue the loop until they are empty.
        if (b->unmounting_ && b->work_queue_.is_empty() && b->producer_queue_.is_empty()) {
            ZX_DEBUG_ASSERT(b->work_queue_.is_empty());
            ZX_DEBUG_ASSERT(b->producer_queue_.is_empty());
            b->state_ = WritebackState::kComplete;
            b->lock_.Release();
            return 0;
        }

        cnd_wait(&b->work_added_, b->lock_.GetInternal());
    }
}

zx_status_t EnqueuePaginated(fbl::unique_ptr<WritebackWork>* work,
                             TransactionManager* transaction_manager, Blob* vn,
                             const zx::vmo& vmo, uint64_t relative_block, uint64_t absolute_block,
                             uint64_t nblocks) {
    const size_t kMaxChunkBlocks = (3 * transaction_manager->WritebackCapacity()) / 4;
    uint64_t delta_blocks = fbl::min(nblocks, kMaxChunkBlocks);
    while (nblocks > 0) {
        if ((*work)->BlkCount() + delta_blocks > kMaxChunkBlocks) {
            // If enqueueing these blocks could push us past the writeback buffer capacity
            // when combined with all previous writes, break this transaction into a smaller
            // chunk first.
            fbl::unique_ptr<WritebackWork> tmp;
            zx_status_t status = transaction_manager->CreateWork(&tmp, vn);
            if (status != ZX_OK) {
                return status;
            }
            if ((status = transaction_manager->EnqueueWork(std::move(*work),
                                                           EnqueueType::kData)) != ZX_OK) {
                return status;
            }
            *work = std::move(tmp);
        }

        (*work)->Enqueue(vmo, relative_block, absolute_block, delta_blocks);
        relative_block += delta_blocks;
        absolute_block += delta_blocks;
        nblocks -= delta_blocks;
        delta_blocks = fbl::min(nblocks, kMaxChunkBlocks);
    }
    return ZX_OK;
}

} // namespace blobfs

// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <limits.h>
#include <stdint.h>

#include <utility>

#include <fbl/unique_fd.h>
#include <fbl/unique_ptr.h>
#include <gpt/gpt.h>

namespace {

constexpr uint32_t kBlockSize = 512;
constexpr uint64_t kBlockCount = 1 << 20;
constexpr uint64_t kAccptableMinimumSize = kBlockSize * kBlockCount;
constexpr uint64_t kGptMetadataSize = 1 << 18; // 256KiB for now. See comment in LibGptTest::Init

static_assert(kGptMetadataSize <= kAccptableMinimumSize,
              "GPT size greater than kAccptableMinimumSize");

class LibGptTest {
public:
    LibGptTest(bool use_ramdisk)
        : use_ramdisk_(use_ramdisk) {}
    ~LibGptTest() {}

    // Creates a ramdisk and initialize GPT on it.
    bool Init();

    // Removes the backing ramdisk device.
    bool Teardown();

    // Returns total size of the disk under test.
    uint64_t GetDiskSize() const {
        return blk_size_ * blk_count_;
    }

    // Return block size of the disk under test.
    uint32_t GetBlockSize() const {
        return blk_size_;
    }

    // Returns total number of block in the disk.
    uint64_t GetBlockCount() const {
        return blk_count_;
    }

    // Return total number of block free in disk after GPT is created.
    uint64_t GetUsableBlockCount() const {
        return usable_last_block_ - usable_start_block_;
    }

    // First block that is free to use after GPT is created.
    uint64_t GetUsableStartBlock() const {
        return usable_start_block_;
    }

    // Last block that is free to use after GPT is created.
    uint64_t GetUsableLastBlock() const {
        return usable_last_block_;
    }

    // Returns number of block GPT uses.
    uint64_t GptMetadataBlocksCount() const {
        // See comment in LibGptTest::Init
        return kGptMetadataSize / blk_size_;
    }

    // Returns the full device path.
    const char* GetDevicePath() const { return disk_path_; }

    // Remove all partition from GPT and keep device in GPT initialized state.
    bool Reset();

    // Finalize uninitialized disk and verify.
    bool Finalize();

    // Sync and verify.
    bool Sync();

    // Get the Range from GPT.
    bool ReadRange();

    // Prepare disk to run Add Partition tests.
    // 1. initialize GPT
    // 2. optionally sync
    // 3. get the usable range
    bool PrepDisk(bool sync);

    // gpt_ changes across Reset(). So we do not expose pointer to GptDevice to
    // any of the test. Instead we expose following wrapper funtions for
    // a few GptDevice methods.
    bool IsGptValid() const { return gpt_->Valid(); }
    zx_status_t GetDiffs(uint32_t partition_index, uint32_t* diffs) const {
        return gpt_->GetDiffs(partition_index, diffs);
    }

    // Get's a partition at index pindex.
    gpt_partition_t* GetPartition(uint32_t pindex) const {
        return gpt_->GetPartition(pindex);
    }

    // Adds a partition
    zx_status_t AddPartition(const char* name, const uint8_t* type, const uint8_t* guid,
                             uint64_t offset, uint64_t blocks, uint64_t flags) {
        return gpt_->AddPartition(name, type, guid, offset, blocks, flags);
    }

    // removes a partition.
    zx_status_t RemovePartition(const uint8_t* guid) { return gpt_->RemovePartition(guid); }

    // removes all partitions.
    zx_status_t RemoveAllPartitions() { return gpt_->RemoveAllPartitions(); }

private:
    // Initialize a physical media.
    bool InitDisk(const char* disk_path);

    // Create and initialize and ramdisk.
    bool InitRamDisk();

    // Teardown the disk.
    bool TearDownDisk();

    // Teardown and destroy ram disk.
    bool TearDownRamDisk();

    // Block size of the device.
    uint32_t blk_size_ = kBlockSize;

    // Number of block in the disk.
    uint64_t blk_count_ = kBlockCount;

    // disk path
    char disk_path_[PATH_MAX] = {};

    // pointer to read GptDevice.
    fbl::unique_ptr<gpt::GptDevice> gpt_;

    // Open file descriptor to block device.
    fbl::unique_fd fd_;

    // Create and use ramdisk instead of a physical disk.
    bool use_ramdisk_;

    // An optional ramdisk structure, which is only non-nullptr if
    // |use_ramdisk_| is true.
    struct ramdisk_client* ramdisk_ = nullptr;

    // usable start block offset.
    uint64_t usable_start_block_ = UINT64_MAX;

    // usable last block offset.
    uint64_t usable_last_block_ = UINT64_MAX;
};

} // namespace

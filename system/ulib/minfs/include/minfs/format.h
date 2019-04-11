// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file describes the on-disk format of MinFS

#pragma once

#include <bitmap/raw-bitmap.h>
#include <bitmap/storage.h>
#include <fbl/macros.h>

#include <zircon/types.h>

#include <assert.h>
#include <limits.h>
#include <limits>
#include <stdbool.h>
#include <stdint.h>

// clang-format off

namespace minfs {

// Type of a reference to block number, either absolute (able to index
// into disk directly) or relative to some entity (such as a file).
typedef uint32_t blk_t;

// The type of an inode number, which may be used as an
// index into the inode table.
typedef uint32_t ino_t;

constexpr uint64_t kMinfsMagic0         = (0x002153466e694d21ULL);
constexpr uint64_t kMinfsMagic1         = (0x385000d3d3d3d304ULL);
constexpr uint32_t kMinfsVersion        = 0x00000007;

constexpr ino_t    kMinfsRootIno        = 1;
constexpr uint32_t kMinfsFlagClean      = 0x00000001; // Currently unused
constexpr uint32_t kMinfsFlagFVM        = 0x00000002; // Mounted on FVM
constexpr uint32_t kMinfsBlockSize      = 8192;
constexpr uint32_t kMinfsBlockBits      = (kMinfsBlockSize * 8);
constexpr uint32_t kMinfsInodeSize      = 256;
constexpr uint32_t kMinfsInodesPerBlock = (kMinfsBlockSize / kMinfsInodeSize);

constexpr uint32_t kMinfsDirect         = 16;
constexpr uint32_t kMinfsIndirect       = 31;
constexpr uint32_t kMinfsDoublyIndirect = 1;

constexpr uint32_t kMinfsDirectPerIndirect  = (kMinfsBlockSize / sizeof(blk_t));
constexpr uint32_t kMinfsDirectPerDindirect = kMinfsDirectPerIndirect * kMinfsDirectPerIndirect;
// not possible to have a block at or past this one
// due to the limitations of the inode and indirect blocks
// constexpr uint64_t kMinfsMaxFileBlock = (kMinfsDirect +
//                                         (kMinfsIndirect * kMinfsDirectPerIndirect)
//                                         + (kMinfsDoublyIndirect * kMinfsDirectPerIndirect
//                                         * kMinfsDirectPerIndirect));
// TODO(ZX-1523): Remove this artificial cap when MinFS can safely deal
// with files larger than 4GB.
constexpr uint64_t kMinfsMaxFileBlock = (std::numeric_limits<uint32_t>::max() / kMinfsBlockSize)
                                        - 1;
constexpr uint64_t kMinfsMaxFileSize  = kMinfsMaxFileBlock * kMinfsBlockSize;

constexpr uint32_t kMinfsTypeFile = 8;
constexpr uint32_t kMinfsTypeDir  = 4;

constexpr uint32_t MinfsMagic(uint32_t T) { return 0xAA6f6e00 | T; }
constexpr uint32_t kMinfsMagicDir  = MinfsMagic(kMinfsTypeDir);
constexpr uint32_t kMinfsMagicFile = MinfsMagic(kMinfsTypeFile);
constexpr uint32_t MinfsMagicType(uint32_t n) { return n & 0xFF; }

constexpr size_t kFVMBlockInodeBmStart = 0x10000;
constexpr size_t kFVMBlockDataBmStart  = 0x20000;
constexpr size_t kFVMBlockInodeStart   = 0x30000;
constexpr size_t kFVMBlockJournalStart = 0x40000;
constexpr size_t kFVMBlockDataStart    = 0x50000;

constexpr blk_t kJournalEntryHeaderMaxBlocks = 2040;

struct Superblock {
    uint64_t magic0;
    uint64_t magic1;
    uint32_t version;
    uint32_t flags;
    uint32_t block_size;    // 8K typical
    uint32_t inode_size;    // 256
    uint32_t block_count;   // total number of data blocks
    uint32_t inode_count;   // total number of inodes
    uint32_t alloc_block_count; // total number of allocated data blocks
    uint32_t alloc_inode_count; // total number of allocated inodes
    blk_t ibm_block;           // first blockno of inode allocation bitmap
    blk_t abm_block;           // first blockno of block allocation bitmap
    blk_t ino_block;           // first blockno of inode table
    blk_t journal_start_block; // first blockno available for journal
    blk_t dat_block;           // first blockno available for file data
    // The following flags are only valid with (flags & kMinfsFlagFVM):
    uint64_t slice_size;     // Underlying slice size
    uint64_t vslice_count;   // Number of allocated underlying slices
    uint32_t ibm_slices;     // Slices allocated to inode bitmap
    uint32_t abm_slices;     // Slices allocated to block bitmap
    uint32_t ino_slices;     // Slices allocated to inode table
    uint32_t journal_slices; // Slices allocated to journal section
    uint32_t dat_slices;     // Slices allocated to file data section

    ino_t unlinked_head;    // Index to the first unlinked (but open) inode.
    ino_t unlinked_tail;    // Index to the last unlinked (but open) inode.
};

static_assert(sizeof(Superblock) <= kMinfsBlockSize,
              "minfs info size is wrong");
// Notes:
// - the inode bitmap, block bitmap, inode table, journal, and data
//   regions must be in that order and may not overlap
// - the abm has an entry for every block on the volume, including
//   the info block (0), the bitmaps, etc
// - data blocks referenced from direct and indirect block tables
//   in inodes are also relative to (0), but it is not legal for
//   a block number of less than dat_block (start of data blocks)
//   to be used
// - inode numbers refer to the inode in block:
//     ino_block + ino / kMinfsInodesPerBlock
//   at offset: ino % kMinfsInodesPerBlock
// - inode 0 is never used, should be marked allocated but ignored

constexpr uint64_t kJournalMagic = (0x6d696e6a75726e6cULL);

// The minimal number of slices to allocate a MinFS partition:
// Superblock, Inode bitmap, Data bitmap, Inode Table, Journal (2), and actual data.
constexpr size_t kMinfsMinimumSlices = 7;

constexpr uint64_t kMinfsDefaultInodeCount = 32768;

struct JournalInfo {
    uint64_t magic;
    uint64_t reserved0;
    uint64_t reserved1;
    uint64_t reserved2;
    uint64_t reserved3;
};

static_assert(sizeof(JournalInfo) <= kMinfsBlockSize, "Journal info size is too large");

struct Inode {
    uint32_t magic;
    uint32_t size;
    uint32_t block_count;
    uint32_t link_count;
    uint64_t create_time;
    uint64_t modify_time;
    uint32_t seq_num;               // bumped when modified
    uint32_t gen_num;               // bumped when deleted
    uint32_t dirent_count;          // for directories
    ino_t last_inode;               // index to the previous unlinked inode
    ino_t next_inode;               // index to the next unlinked inode
    uint32_t rsvd[3];
    blk_t dnum[kMinfsDirect];    // direct blocks
    blk_t inum[kMinfsIndirect];  // indirect blocks
    blk_t dinum[kMinfsDoublyIndirect]; // doubly indirect blocks
};

static_assert(sizeof(Inode) == kMinfsInodeSize,
              "minfs inode size is wrong");

struct Dirent {
    ino_t ino;                      // inode number
    uint32_t reclen;                // Low 28 bits: Length of record
                                    // High 4 bits: Flags
    uint8_t namelen;                // length of the filename
    uint8_t type;                   // kMinfsType*
    char name[];                    // name does not have trailing \0
};

constexpr uint32_t MINFS_DIRENT_SIZE = sizeof(Dirent);

constexpr uint32_t DirentSize(uint8_t namelen) {
    return MINFS_DIRENT_SIZE + ((namelen + 3) & (~3));
}

constexpr uint8_t kMinfsMaxNameSize       = 255;
// The largest acceptable value of DirentSize(dirent->namelen).
// The 'dirent->reclen' field may be larger after coalescing
// entries.
constexpr uint32_t kMinfsMaxDirentSize    = DirentSize(kMinfsMaxNameSize);
constexpr uint32_t kMinfsMaxDirectorySize = (((1 << 20) - 1) & (~3));

static_assert(kMinfsMaxNameSize >= NAME_MAX,
              "MinFS names must be large enough to hold NAME_MAX characters");

constexpr uint32_t kMinfsReclenMask = 0x0FFFFFFF;
constexpr uint32_t kMinfsReclenLast = 0x80000000;

constexpr uint32_t MinfsReclen(Dirent* de, size_t off) {
    return (de->reclen & kMinfsReclenLast) ?
           kMinfsMaxDirectorySize - static_cast<uint32_t>(off) :
           de->reclen & kMinfsReclenMask;
}

static_assert(kMinfsMaxDirectorySize <= kMinfsReclenMask,
              "MinFS directory size must be smaller than reclen mask");

// Notes:
// - dirents with ino of 0 are free, and skipped over on lookup
// - reclen must be a multiple of 4
// - the last record in a directory has the "kMinfsReclenLast" flag set. The
//   actual size of this record can be computed from the offset at which this
//   record starts. If the MAX_DIR_SIZE is increased, this 'last' record will
//   also increase in size.

// blocksize   8K    16K    32K
// 16 dir =  128K   256K   512K
// 32 ind =  512M  1024M  2048M

//  1GB ->  128K blocks ->  16K bitmap (2K qword)
//  4GB ->  512K blocks ->  64K bitmap (8K qword)
// 32GB -> 4096K blocks -> 512K bitmap (64K qwords)

// Block Cache (bcache.c)
constexpr uint32_t kMinfsHashBits = (8);

} // namespace minfs

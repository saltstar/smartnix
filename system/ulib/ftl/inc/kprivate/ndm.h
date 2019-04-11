// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <targetos.h>
#include <kernel.h>
#include <fsdriver.h>

/***********************************************************************/
/* Symbol Definitions                                                  */
/***********************************************************************/
#define NDM_PART_NAME_LEN 15            // partition name size in bytes
#define NDM_PART_USER 0                 // number of ui32s in partition for user
#define NDM_PART_NAME NDM_PART_NAME_LEN // obsolete symbol

// Various NAND device types
#define NDM_SLC (1 << 0)
#define NDM_MLC (1 << 1)
#define NDM_WR1 (1 << 2)

// Various function return types
#define NDM_INIT_BAD_BLOCK 1
#define NDM_CTRL_BLOCK 2
#define NDM_REG_BLOCK 3

// Various states for a page - used by data_and_spare_check()
#define NDM_PAGE_ERASED 0
#define NDM_PAGE_VALID 1
#define NDM_PAGE_INVALID 2

// write_data_and_spare action parameter values
#define NDM_NONE 0
#define NDM_ECC 1
#define NDM_ECC_VAL 2

/***********************************************************************/
/* Type Declarations                                                   */
/***********************************************************************/
// NDM Partition Information
typedef struct {
    ui32 first_block; // first virtual block for partition
    ui32 num_blocks;  // number of virtual blocks in partition
#if NDM_PART_USER
    ui32 user[NDM_PART_USER]; // reserved for the user
#endif
    char name[NDM_PART_NAME_LEN]; // partition name
    ui8 type;                     // partition type - same as vstat()
} NDMPartition;

// Driver count statistics for TargetNDM devices
typedef struct {
    ui32 write_page;      // number of write_data_and_spare() calls
    ui32 write_pages;     // number of write_pages() calls
    ui32 read_page;       // number of read_decode_data() calls
    ui32 read_pages;      // number of read_pages() calls
    ui32 xfr_page;        // number of transfer_page() calls
    ui32 read_dec_spare;  // number of read_decode_spare() calls
    ui32 read_spare;      // number of read_spare() calls
    ui32 page_erased;     // number of data_and_spare_erased() calls
    ui32 check_page;      // number of data_and_spare_check() calls
    ui32 erase_block;     // number of erase_block() calls
    ui32 is_block_bad;    // number of is_block_bad() calls
} NdmDvrStats;

// Driver Interface Structure
typedef struct {
    ui32 num_blocks;     // total number of blocks on device
    ui32 max_bad_blocks; // maximum number of bad blocks
    ui32 block_size;     // block size in bytes
    ui32 page_size;      // page data area in bytes
    ui32 eb_size;        // used spare area in bytes
    ui32 flags;          // option flags
    ui32 type;           // type of device
    void* dev;           // optional value set by driver

    // Driver Functions
    int (*write_data_and_spare)(ui32 pn, const ui8* data, ui8* spare, int action, void* dev);
    int (*write_pages)(ui32 pn, ui32 count, const ui8* data, ui8* spare, int action, void* dev);
    int (*read_decode_data)(ui32 pn, ui8* data, ui8* spare, void* dev);
    int (*read_pages)(ui32 pn, ui32 count, ui8* data, ui8* spare, void* dev);
    int (*transfer_page)(ui32 old_pn, ui32 new_pn, ui8* data, ui8* old_spare, ui8* new_spare,
                         int encode_spare, void* dev);
#if INC_FFS_NDM_MLC || INC_FTL_NDM_MLC
    ui32 (*pair_offset)(ui32 page_offset, void* dev);
#endif
    int (*read_decode_spare)(ui32 pn, ui8* spare, void* dev);
    int (*read_spare)(ui32 pn, ui8* spare, void* dev);
    int (*data_and_spare_erased)(ui32 pn, ui8* data, ui8* spare, void* dev);
    int (*data_and_spare_check)(ui32 pn, ui8* data, ui8* spare, int* status, void* dev);
    int (*erase_block)(ui32 pn, void* dev);
    int (*is_block_bad)(ui32 pn, void* dev);
#if FS_DVR_TEST
    ui32 dev_eb_size; /* device spare area size */
    void (*chip_show)(void* vol);
    int (*rd_raw_spare)(ui32 p, ui8* spare, void* dev);
    int (*rd_raw_page)(ui32 p, void* data, void* dev);
#endif
} NDMDrvr;

// NDM Control Block
typedef struct ndm* NDM;
typedef const struct ndm* CNDM;

/***********************************************************************/
/* Functions Prototypes                                                */
/***********************************************************************/
// General API
NDM ndmAddDev(const NDMDrvr* drvr);
int ndmDelDev(NDM ndm);
ui32 ndmGetNumVBlocks(CNDM ndm);
int ndmUnformat(NDM ndm);
ui32 ndmPastPrevPair(CNDM ndm, ui32 pn);

// Partitions API
ui32 ndmGetNumPartitions(CNDM ndm);
int ndmSetNumPartitions(NDM ndm, ui32 num_partitions);
int ndmReadPartition(CNDM ndm, NDMPartition* part, ui32 part_num);
const NDMPartition* ndmGetPartition(CNDM ndm, ui32 part_num);
int ndmWritePartition(NDM ndm, const NDMPartition* part, ui32 part_num, const char* name);
int ndmErasePartition(NDM ndm, ui32 part_num);
void ndmDeletePartition(CNDM ndm, ui32 part_num);
void ndmDeletePartitionTable(NDM ndm);
int ndmSavePartitionTable(NDM ndm);
int ndmDelVols(CNDM ndm);
int ndmDelVol(CNDM ndm, ui32 part_num);
int ndmWrFatPartition(NDM ndm, ui32 part_num);

// User Volume API
int ndmEraseBlock(ui32 pn, void* ndm_ptr);
int ndmReadPages(ui32 start_pn, ui32 count, void* data, void* spare, void* ndm_ptr);
int ndmWritePages(ui32 start_pn, ui32 count, const void* data, void* spare, void* ndm_ptr);

// FAT/XFS/FFS Volume API
int ndmAddVolFatFTL(NDM ndm, ui32 part_no, FtlNdmVol* ftl, FatVol* fat);
int ndmAddVolXfsFTL(NDM ndm, ui32 part_no, FtlNdmVol* ftl, XfsVol* xfs);
int ndmAddVolFFS(NDM ndm, ui32 part_num, FfsVol* ffs_dvr);

// Driver Test/Special Routines
int ndmExtractBBL(NDM ndm);
int ndmInsertBBL(NDM ndm);
int NdmDvrTestAdd(const NDMDrvr* dev);

#ifdef __cplusplus
}
#endif

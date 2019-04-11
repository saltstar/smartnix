// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ftlnp.h"

#if INC_FTL_NDM
// Configuration
#ifndef FTLN_DEBUG_RECYCLES
#define FTLN_DEBUG_RECYCLES FALSE
#endif
#define WC_LAG_LIM1 200 // periodic wear priority trigger
#define WC_LAG_LIM2 252 // constant wear priority trigger

// Type Definitions
typedef struct {
    ui32 vsn0;
    ui32 cnt;
    const ui8* buf;
} SectWr;

typedef struct {
    ui32 vpn0;
    ui32 ppn0;
    ui32 cnt;
    const ui8* buf;
} PageWr;

// Function Prototypes
#ifdef FTL_RESUME_STRESS
void FtlNumFree(uint num_free_blks);
#endif
#if FTLN_DEBUG_RECYCLES
static int recycle_possible(CFTLN ftl, ui32 b);
static ui32 block_selector(FTLN ftl, ui32 b);

// Global Variable Definitions
int FtlnShow, MaxCnt;

// Local Function Definitions

//    show_blk: Show the state of a block using its blocks[] entry
//
//      Inputs: ftl = pointer to FTL control block
//              b = block number
//
static int show_blk(FTLN ftl, ui32 b) {
    int n;

    n = printf("b%2d ", b);
    if (IS_FREE(ftl->bdata[b])) {
        if (ftl->bdata[b] & ERASED_BLK_FLAG)
            n += printf("erased");
        else
            n += printf("free  ");
        n += printf(" wl=%d ", ftl->blk_wc_lag[b]);
        return n;
    } else if (IS_MAP_BLK(ftl->bdata[b])) {
        putchar('m');
        ++n;
    } else {
        putchar('v');
        ++n;
    }
    n += printf(" u=%-2d", NUM_USED(ftl->bdata[b]));
    n += printf(" wl=%d ", ftl->blk_wc_lag[b]);
    if (!recycle_possible(ftl, b))
        n += printf("np");
    else
        n += printf("s=%d", block_selector(ftl, b));
    if (ftl->free_vpn / ftl->pgs_per_blk == b)
        n += printf(" FV");
    else if (ftl->free_mpn / ftl->pgs_per_blk == b)
        n += printf(" FM");
    return n;
}

//   show_blks: List the number free and the state of each block
//
//       Input: ftl = pointer to FTL control block
//
static void show_blks(FTLN ftl) {
    int n, l, q = (ftl->num_blks + 1) / 4;

    printf("num_free=%d", ftl->num_free_blks);
    for (l = 0; l < q; ++l) {
        putchar('\n');
        n = show_blk(ftl, l);
        if (l + q <= ftl->num_blks) {
            Spaces(31 - n);
            n = show_blk(ftl, l + q);
        }
        if (l + 2 * q <= ftl->num_blks) {
            Spaces(31 - n);
            n = show_blk(ftl, l + 2 * q);
        }
        if (l + 3 * q <= ftl->num_blks) {
            Spaces(31 - n);
            n = show_blk(ftl, l + 3 * q);
        }
    }
}
#endif // FTLN_DEBUG_RECYCLES

// next_free_vpg: Get next free volume page
//
//       Input: ftl = pointer to FTL control block
//
//     Returns: page number if successful, else (ui32)-1 if error
//
static ui32 next_free_vpg(FTLN ftl) {
    ui32 pn;

    // If needed, allocate new volume block if needed.
    if (ftl->free_vpn == (ui32)-1) {
        ui32 b;

        // Find free block with highest wear count. Error if none are free.
        b = FtlnHiWcFreeBlk(ftl);
        if (b == (ui32)-1)
            return b;

        // If the block is unerased, erase it now. Return -1 if error.
        if ((ftl->bdata[b] & ERASED_BLK_FLAG) == 0)
            if (FtlnEraseBlk(ftl, b))
                return (ui32)-1;

        // Decrement free block count.
        PfAssert(ftl->num_free_blks);
        --ftl->num_free_blks;
#ifdef FTL_RESUME_STRESS
        FtlNumFree(ftl->num_free_blks);
#endif

        // Set free volume page pointer to first page in block.
        ftl->free_vpn = b * ftl->pgs_per_blk;

        // Clear block's free/erased flags and read count.
        ftl->bdata[b] = 0;  // clr free flag, used/read pages cnts
    }

    // Allocate free volume page. If end of block, invalidate free ptr.
    pn = ftl->free_vpn++;
    if (ftl->free_vpn % ftl->pgs_per_blk == 0)
        ftl->free_vpn = (ui32)-1;

    // Return allocate page number.
    return pn;
}

// next_free_mpg: Get next free map page
//
//       Input: ftl = pointer to FTL control block
//
//     Returns: page number if successful, else (ui32)-1 if error
//
static ui32 next_free_mpg(FTLN ftl) {
    ui32 pn;

    // If needed, allocate new map block.
    if (ftl->free_mpn == (ui32)-1) {
        ui32 b;

        // Find free block with lowest wear count. Error if none are free.
        b = FtlnLoWcFreeBlk(ftl);
        if (b == (ui32)-1)
            return b;

        // If the block is unerased, erase it now. Return -1 if error.
        if ((ftl->bdata[b] & ERASED_BLK_FLAG) == 0)
            if (FtlnEraseBlk(ftl, b))
                return (ui32)-1;

        // Decrement free block count.
        PfAssert(ftl->num_free_blks);
        --ftl->num_free_blks;
#ifdef FTL_RESUME_STRESS
        FtlNumFree(ftl->num_free_blks);
#endif

        // Set free MPN pointer to first page in block and inc block count.
        ftl->free_mpn = b * ftl->pgs_per_blk;
        ++ftl->high_bc;

        // Clear free block flag and read count, set map block flag.
        SET_MAP_BLK(ftl->bdata[b]);  // clr free flag & wear/read count
    }

    // Use first page on free map page list.
    pn = ftl->free_mpn;

// Move to next writable page. If MLC flash, that is page whose pair
// has a higher offset. Invalidate index if end of block reached.
#if INC_FTL_NDM_MLC && (INC_FTL_NDM_SLC || INC_FTL_NOR_WR1)
    if (ftl->type == NDM_MLC)
#endif
#if INC_NDM_MLC
        for (;;) {
            ui32 pg_offset = ++ftl->free_mpn % ftl->pgs_per_blk;

            if (pg_offset == 0) {
                ftl->free_mpn = (ui32)-1;
                break;
            }
            if (ftl->pair_offset(pg_offset, ftl->ndm) >= pg_offset)
                break;
        }
#endif
#if INC_FTL_NDM_MLC && (INC_FTL_NDM_SLC || INC_FTL_NOR_WR1)
    else
#endif
#if INC_FTL_NDM_SLC || INC_FTL_NOR_WR1
        if (++ftl->free_mpn % ftl->pgs_per_blk == 0)
        ftl->free_mpn = (ui32)-1;
#endif

#if INC_FTL_NDM_MLC && FS_ASSERT
    // For MLC devices, sanity check that this is a safe write.
    if (ftl->type == NDM_MLC) {
        ui32 pg_offset = pn % ftl->pgs_per_blk;

        PfAssert(ftl->pair_offset(pg_offset, ftl->ndm) >= pg_offset);
    }
#endif

    // Return allocated page number.
    return pn;
}

// wr_vol_page: Write a volume page to flash
//
//      Inputs: ftl = pointer to FTL control block
//              vpn = virtual page number
//              buf = pointer to page data buffer or NULL
//              old_pn = old location for page, if any
//
//     Returns: 0 on success, -1 on error
//
static int wr_vol_page(FTLN ftl, ui32 vpn, void* buf, ui32 old_pn) {
    ui32 ppn, b, wc;
    int rc;

#if INC_ELIST
    // If list of erased blocks/wear counts exists, erase it now.
    if (ftl->elist_blk != (ui32)-1)
        if (FtlnEraseBlk(ftl, ftl->elist_blk))
            return -1;
#endif

    // Allocate next free volume page. Return -1 if error.
    ppn = next_free_vpg(ftl);
    if (ppn == (ui32)-1)
        return -1;

    // Calculate the block's erase wear count.
    b = ppn / ftl->pgs_per_blk;
    wc = ftl->high_wc - ftl->blk_wc_lag[b];

    // Initialize spare area, including VPN and block wear count.
    memset(ftl->spare_buf, 0xFF, ftl->eb_size);
    SET_SA_VPN(vpn, ftl->spare_buf);
    SET_SA_WC(wc, ftl->spare_buf);

    // If page data in buffer, write it. Returns 0 or -2.
    if (buf) {
        ++ftl->stats.write_page;
        rc = ftl->write_page(ftl->start_pn + ppn, buf, ftl->spare_buf, ftl->ndm);
    }

    // Else invoke page transfer routine. Returns 0, -2, or 1.
    else {
        ++ftl->stats.transfer_page;
        rc = ftl->xfer_page(ftl->start_pn + old_pn, ftl->start_pn + ppn, ftl->main_buf,
                            ftl->spare_buf, ftl->ndm);
    }

    // Return -1 for any error. Any write error is fatal.
    if (rc)
        return FtlnFatErr(ftl);

    // Increment block's used pages count.
    PfAssert(!IS_FREE(ftl->bdata[b]) && !IS_MAP_BLK(ftl->bdata[b]));
    INC_USED(ftl->bdata[b]);

    // If page has an older copy, decrement used count on old block.
    if (old_pn != (ui32)-1)
        FtlnDecUsed(ftl, old_pn, vpn);

    // Update mapping for this virtual page. Return status.
    return FtlnMapSetPpn(ftl, vpn, ppn);
}

// wr_map_page: Write a map page to flash
//
//      Inputs: ftl = pointer to FTL control block
//              mpn = map page to write
//              buf = pointer to page data buffer or NULL
//
//     Returns: 0 on success, -1 on failure
//
static int wr_map_page(FTLN ftl, ui32 mpn, void* buf) {
    ui32 pn, b, wc;
    int status;
    ui32 old_pn = ftl->mpns[mpn];

    // Return -1 if fatal I/O error occurred.
    if (ftl->flags & FTLN_FATAL_ERR)
        return FsError(EIO);

#if INC_ELIST
    // If list of erased blocks/wear counts exists, erase it now.
    if (ftl->elist_blk != (ui32)-1)
        if (FtlnEraseBlk(ftl, ftl->elist_blk))
            return -1;
#endif

    // Allocate next free map page. Return -1 if error.
    pn = next_free_mpg(ftl);
    if (pn == (ui32)-1)
        return -1;

    // Determine the block's erase wear count.
    b = pn / ftl->pgs_per_blk;
    wc = ftl->high_wc - ftl->blk_wc_lag[b];

    // Initialize spare area, including VPN, block count, and wear count.
    memset(ftl->spare_buf, 0xFF, ftl->eb_size);
    SET_SA_VPN(mpn, ftl->spare_buf);
    SET_SA_BC(ftl->high_bc, ftl->spare_buf);
    SET_SA_WC(wc, ftl->spare_buf);

    // If page data in buffer, invoke write_page().
    if (buf) {
        ++ftl->stats.write_page;
        status = ftl->write_page(ftl->start_pn + pn, buf, ftl->spare_buf, ftl->ndm);
    }

    // Else source data is in flash. Invoke page transfer routine.
    else {
        ++ftl->stats.transfer_page;
        status = ftl->xfer_page(ftl->start_pn + old_pn, ftl->start_pn + pn, ftl->main_buf,
                                ftl->spare_buf, ftl->ndm);
    }

    // I/O or ECC decode error is fatal.
    if (status)
        return FtlnFatErr(ftl);

    // If the meta page, invalidate pointer to physical location.
    if (mpn == ftl->num_map_pgs - 1)
        ftl->mpns[mpn] = (ui32)-1;

    // Else adjust block used page counts.
    else {
        // Increment page used count in new block.
        PfAssert(IS_MAP_BLK(ftl->bdata[b]));
        INC_USED(ftl->bdata[b]);

        // Set the MPN array entry with the new page number.
        ftl->mpns[mpn] = pn;

        // If page has an older copy, decrement used count on old block.
        if (old_pn != (ui32)-1)
            FtlnDecUsed(ftl, old_pn, mpn);
    }

    // Return success.
    return 0;
}

// free_vol_list_pgs: Return number of free pages on free_vpn block
//
//       Input: ftl = pointer to FTL control block
//
static int free_vol_list_pgs(CFTLN ftl) {
    // If free_vpn has invalid page number, no list of free vol pages.
    if (ftl->free_vpn == (ui32)-1)
        return 0;

    // Use the page offset to get the number of free pages left on block.
    return ftl->pgs_per_blk - ftl->free_vpn % ftl->pgs_per_blk;
}

// free_map_list_pgs: Return number of free pages on free_mpn block
//
//       Input: ftl = pointer to FTL control block
//
static int free_map_list_pgs(CFTLN ftl) {
    int free_mpgs;

    // If free_mpn has invalid page number, no list of free map pages.
    if (ftl->free_mpn == (ui32)-1)
        return 0;

    // Use the page offset to get the number of free pages left on block.
    free_mpgs = ftl->pgs_per_blk - ftl->free_mpn % ftl->pgs_per_blk;

#if INC_FTL_NDM_MLC
    // We only use half the available pages on an MLC map block.
    if (ftl->type == NDM_MLC)
        free_mpgs /= 2;
#endif

    // Return the number of free pages on the free_mpn list.
    return free_mpgs;
}

// recycle_possible: Check if there are enough free blocks to recycle
//              a specified block
//
//      Inputs: ftl = pointer to FTL control block
//              b = potential recycle block to check
//
//     Returns: TRUE if enough free blocks, FALSE otherwise
//
static int recycle_possible(CFTLN ftl, ui32 b) {
    ui32 used, free_mpgs = 0, needed_free;

    // Determine how many used pages the prospective recycle block has.
    used = NUM_USED(ftl->bdata[b]);

    // If block has no used pages and it's a map block or no cached map
    // pages are dirty, no writes are needed and so recycle is possible.
    if ((used == 0) && (IS_MAP_BLK(ftl->bdata[b]) || (ftl->map_cache->num_dirty == 0)))
        return TRUE;

    // If free map page list is empty or on prospective recycle block,
    // need a new free map block, but get a bunch of free map pages.
    if ((ftl->free_mpn == (ui32)-1) || (ftl->free_mpn / ftl->pgs_per_blk == b)) {
        needed_free = 1;
        free_mpgs = ftl->pgs_per_blk;
#if INC_FTL_NDM_MLC
        if (ftl->type == NDM_MLC)  // only use half of MLC map block pages
            free_mpgs /= 2;
#endif
    }

    // Else get number of free map pages. If map block, need free block
    // if free map page count is less than this block's used page count.
    else {
        free_mpgs = free_map_list_pgs(ftl);
        if (IS_MAP_BLK(ftl->bdata[b]))
            needed_free = free_mpgs < used;
        else
            needed_free = 0;
    }

    // If volume block, free blocks may be needed for both volume page
    // transfer and the post-recycle map cache flush.
    if (!IS_MAP_BLK(ftl->bdata[b])) {
        ui32 avail_blk_pgs, map_pgs;

        if ((ftl->free_vpn == (ui32)-1) || (ftl->free_vpn / ftl->pgs_per_blk == b)) {
            // If free volume page list is empty or on the prospective block,
            // need new free volume block.
            ++needed_free;
        } else if ((ui32)free_vol_list_pgs(ftl) < used) {
            // Else if the number of free volume pages is less than the number
            // of used pages on prospective block, need another volume block.
            ++needed_free;
        }

        // Assume each volume page transfer updates a separate map page
        // (worst case). Add the number of dirty cached map pages. If this
        // exceeds the number of free map pages, add needed map blocks.
        map_pgs = used + ftl->map_cache->num_dirty;
        if (map_pgs > free_mpgs) {
            avail_blk_pgs = ftl->pgs_per_blk;
#if INC_FTL_NDM_MLC
            if (ftl->type == NDM_MLC)
                avail_blk_pgs /= 2;
#endif
            needed_free += (map_pgs - free_mpgs + avail_blk_pgs - 1) / avail_blk_pgs;
        }
    }

    // For recovery from worst-case powerfail recovery interruption,
    // recycles must leave one free block for the resume process.
    ++needed_free;

    // Recycles are possible if there are enough free blocks.
    return ftl->num_free_blks >= needed_free;
}

// block_selector: Compute next recycle block selector for a block: a
//              combination of its dirty page count, erase wear count,
//              and read wear count
//
//      Inputs: ftl = pointer to FTL control block
//              b = block to compute selector for
//
//     Returns: Selector used to determine whether block is recycled
//
static ui32 block_selector(FTLN ftl, ui32 b) {
    ui32 blk_pages, priority, wc_lag = ftl->blk_wc_lag[b];

    // Get maximum number of used pages. Only use half of MLC map block.
    blk_pages = ftl->pgs_per_blk;
#if INC_FTL_NDM_MLC
    if (ftl->type == NDM_MLC && IS_MAP_BLK(ftl->bdata[b]))
        blk_pages /= 2;
#endif

    // Free page count is primary effect, 'high_wc' lag is secondary.
    priority = (blk_pages - NUM_USED(ftl->bdata[b])) * 255 + wc_lag;

    // The lines below has not been proven or found to be required, but
    // given the criticality of not running out of free blocks, they are
    // left as a safety valve, to ensure that when the free block count
    // is critically low, the recycle block selection is based only on
    // which block consumes the least number of free pages to free.
    if (ftl->num_free_blks < 3)
        return priority;

    // If block's read count is too high, there is danger of losing its
    // data, so use a maximum priority boost.
    if (GET_RC(ftl->bdata[b]) >= ftl->max_rc)
        return priority + 256 * ftl->pgs_per_blk * 255;

    // Recycling blocks only because of wear count limit is potentially
    // wasted work, because the application may overwrite or delete this
    // data before our maximum wear count lag is reached. So delay as
    // long as possible. At first limit (WC_LAG_LIM1) let wear become
    // primary effect, but not every recycle, to keep performance high.
    // 'deferment', the number of recycles between wear count dominated
    // selections, starts at 4 (heuristic value) and decreases linearly
    // to 1 at the second limit (WC_LAG_LIM2). At that point, moving
    // static data must be the higher priority and so wear becomes the
    // primary effect at every recycle.
    if (wc_lag >= WC_LAG_LIM1) {
        if (wc_lag >= WC_LAG_LIM2)
            priority += wc_lag * ftl->pgs_per_blk * 255;
        else if (ftl->deferment == 0) {
            priority += wc_lag * ftl->pgs_per_blk * 255;
            ftl->deferment = 4 - 3 * (wc_lag - WC_LAG_LIM1) / (WC_LAG_LIM2 - WC_LAG_LIM1);
        }
    }

    // Return priority value (higher value is better to recycle).
    return priority;
}

// next_recycle_blk: Choose next block (volume or map) to recycle
//
//       Input: ftl = pointer to FTL control block
//
//     Returns: Chosen recycle block, (ui32)-1 on error
//
static ui32 next_recycle_blk(FTLN ftl) {
    ui32 b, rec_b, selector, best_selector = 0;

    // Initially set flag as if no block is at the max read-count limit.
    ftl->max_rc_blk = (ui32)-1;

    // Scan blocks to select block to recycle.
    for (b = 0, rec_b = (ui32)-1; b < ftl->num_blks; ++b) {
        // Skip free blocks.
        if (IS_FREE(ftl->bdata[b]))
            continue;

        // Check if block is at read-wear limit.
        if (GET_RC(ftl->bdata[b]) >= ftl->max_rc) {
            // If first block, save its number, else mark as 'some at limit'.
            if (ftl->max_rc_blk == (ui32)-1)
                ftl->max_rc_blk = b;
            else
                ftl->max_rc_blk = (ui32)-2;
        }

        // Else not at read-wear limit, skip blocks holding a free list.
        else if (ftl->free_vpn / ftl->pgs_per_blk == b || ftl->free_mpn / ftl->pgs_per_blk == b)
            continue;

        // If recycle not possible on this block, skip it.
        if (recycle_possible(ftl, b) == FALSE)
            continue;

        // Compute block selector.
        selector = block_selector(ftl, b);

        // If no recycle block selected yet, or if the current block has a
        // higher selector value, remember it. Also, if the selector value
        // is the same, prefer volume blocks over map blocks to avoid map
        // blocks being recycled too often when the volume is full.
        if (rec_b == (ui32)-1 || best_selector < selector ||
            (best_selector == selector && !IS_MAP_BLK(ftl->bdata[b]) &&
             IS_MAP_BLK(ftl->bdata[rec_b]))) {
            rec_b = b;
            best_selector = selector;
        }
    }

    // If no recycle block found, try one of the partially written ones.
    if (rec_b == (ui32)-1) {
        // Check if block holding free volume page pointer can be used.
        if (ftl->free_vpn != (ui32)-1) {
            b = ftl->free_vpn / ftl->pgs_per_blk;
            if (recycle_possible(ftl, b)) {
                rec_b = b;
                best_selector = block_selector(ftl, b);
            }
        }

        // Check if free map page list block can be used and is better.
        if (ftl->free_mpn != (ui32)-1) {
            b = ftl->free_mpn / ftl->pgs_per_blk;
            if (recycle_possible(ftl, b) && block_selector(ftl, b) > best_selector)
                rec_b = b;
        }
    }

    // If one of the partially written blocks has been selected,
    // invalidate the corresponding head of free space.
    if (rec_b != (ui32)-1) {
        if (ftl->free_mpn / ftl->pgs_per_blk == rec_b)
            ftl->free_mpn = (ui32)-1;
        else if (ftl->free_vpn / ftl->pgs_per_blk == rec_b)
            ftl->free_vpn = (ui32)-1;
    }

#if FTLN_DEBUG
    if (rec_b == (ui32)-1) {
        puts("FTL NDM failed to choose next recycle block!");
        FtlnBlkStats(ftl);
    }
#endif

#if FTLN_DEBUG_RECYCLES
    if (FtlnShow)
        printf("\nrec_b=%d\n", rec_b);
#endif

    // Return block chosen for next recycle.
    return rec_b;
}

// recycle_vblk: Recycle one volume block
//
//      Inputs: ftl = pointer to FTL control block
//              recycle_b = block to be recycled
//
//     Returns: 0 on success, -1 on error
//
static int recycle_vblk(FTLN ftl, ui32 recycle_b) {
    ui32 pn, past_end;

#if FTLN_DEBUG > 1
    printf("recycle_vblk: block# %u\n", recycle_b);
#endif

    // Transfer all used pages from recycle block to free block.
    pn = recycle_b * ftl->pgs_per_blk;
    past_end = pn + ftl->pgs_per_blk;
    for (; NUM_USED(ftl->bdata[recycle_b]); ++pn) {
        int rc;
        ui32 vpn, pn2;

        // Error if looped over block and not enough valid used pages.
        if (pn >= past_end)
            return FtlnFatErr(ftl);

        // Read page's spare area.
        ++ftl->stats.read_spare;
        rc = ftl->read_spare(ftl->start_pn + pn, ftl->spare_buf, ftl->ndm);

        // Return -1 if fatal error, skip page if ECC error on spare read.
        if (rc) {
            if (rc == -2)
                return FtlnFatErr(ftl);
            else
                continue;
        }

        // Get virtual page number from spare. Skip page if unmapped.
        vpn = GET_SA_VPN(ftl->spare_buf);
        if (vpn > ftl->num_vpages)
            continue;

        // Retrieve physical page number for VPN. Return -1 if error.
        if (FtlnMapGetPpn(ftl, vpn, &pn2) < 0)
            return -1;

        // Skip page copy if physical mapping is outdated.
        if (pn2 != pn)
            continue;

        // Write page to new flash block. Return -1 if error.
        if (wr_vol_page(ftl, vpn, NULL, pn))
            return -1;
    }

    // Save MPGs modified by volume page transfers. Return -1 if error.
    if (ftlmcFlushMaps(ftl->map_cache))
        return -1;

#if INC_FTL_NDM_MLC
    // For MLC devices, advance free_vpn pointer so next volume page
    // write can't corrupt previously written valid page.
    FtlnMlcSafeFreeVpn(ftl);
#endif

    // Mark recycle block as free. Increment free block count.
    ftl->bdata[recycle_b] = FREE_BLK_FLAG;
    ++ftl->num_free_blks;

    // If this is last block at RC limit, flag that none are at limit.
    if (ftl->max_rc_blk == recycle_b)
        ftl->max_rc_blk = (ui32)-1;

    // Return success.
    return 0;
}

//     recycle: Perform a block recycle
//
//       Input: ftl = pointer to FTL control block
//
//     Returns: 0 on success, -1 on error
//
static int recycle(FTLN ftl) {
    ui32 rec_b;

#if FTLN_DEBUG_RECYCLES
    // If enabled, list the number free and the state of each block.
    if (FtlnShow) {
        printf("num_free=%d\n", ftl->num_free_blks);
        show_blks(ftl);
    }
#endif

    // Confirm no physical page number changes in critical sections.
    PfAssert(ftl->assert_no_recycle == FALSE);

    // Select next block to recycle. Return error if unable.
    rec_b = next_recycle_blk(ftl);
    if (rec_b == (ui32)-1) {
        PfAssert(FALSE); //lint !e506, !e774
        return FsError(ENOSPC);
    }

    // Recycle the block. Return status.
    if (IS_MAP_BLK(ftl->bdata[rec_b]))
        return FtlnRecycleMapBlk(ftl, rec_b);
    else
        return recycle_vblk(ftl, rec_b);
}

#if INC_SECT_FTL
// partial_page_write: Write a virtual page that has been only
//              partially modified
//
//      Inputs: ftl = pointer to FTL control block
//              sect_wr = pointer to structure holding 1st VSN, sector
//                       count, and output buffer pointer
//
//     Returns: 0 on success, -1 on failure
//
static int partial_page_write(FTLN ftl, SectWr* sect_wr) {
    ui32 vpn, old_pn, grp_cnt, grp_i;
#if INC_FTL_PAGE_CACHE
    FcEntry* ftlvc_ent;
#endif

    // Compute first virtual page number.
    vpn = sect_wr->vsn0 / ftl->sects_per_page;

    // Compute number of sectors to be written in page.
    grp_i = sect_wr->vsn0 % ftl->sects_per_page;
    grp_cnt = ftl->sects_per_page - grp_i;
    if (grp_cnt > sect_wr->cnt)
        grp_cnt = sect_wr->cnt;

#if INC_FTL_PAGE_CACHE
    // Check if there is a virtual pages cache and this page is in it.
    if (ftl->vol_cache && (ftlvc_ent = FcGetEntry(ftl->vol_cache, vpn, 0, NULL)) != NULL) {
        // Update page contents.
        memcpy(&ftlvc_ent->data[grp_i * ftl->sect_size], sect_wr->buf, grp_cnt * ftl->sect_size);

        // Mark cache entry dirty and unpin it.
        ftlvcSetPageDirty(ftl->vol_cache, ftlvc_ent);
        FcFreeEntry(ftl->vol_cache, &ftlvc_ent);
    }

    // Else update page in flash.
    else
#endif
    {
        // Prepare to write one volume page.
        if (FtlnRecCheck(ftl, 1))
            return -1;

        // Retrieve physical page number for VPN. Return -1 if error.
        if (FtlnMapGetPpn(ftl, vpn, &old_pn) < 0)
            return -1;

#if FS_ASSERT
        // Confirm no physical page number changes below.
        ftl->assert_no_recycle = TRUE;
#endif

        // If unmapped, fill page with 0xFF.
        if (old_pn == (ui32)-1)
            memset(ftl->swap_page, 0xFF, ftl->page_size);

        // Else read page contents in. Return -1 if error.
        else if (FtlnRdPage(ftl, old_pn, ftl->swap_page))
            return -1;

        // Update contents of page with new data to be written.
        memcpy(&ftl->swap_page[grp_i * ftl->sect_size], sect_wr->buf, grp_cnt * ftl->sect_size);

        // Write page to flash. Return -1 if error.
        if (wr_vol_page(ftl, vpn, ftl->swap_page, old_pn))
            return -1;
        PfAssert(ftl->num_free_blks >= FTLN_MIN_FREE_BLKS);

#if FS_ASSERT
        // End check for no physical page number changes.
        ftl->assert_no_recycle = FALSE;
#endif
    }

    // Adjust input parameters based on number of sectors written.
    sect_wr->buf += grp_cnt * ftl->sect_size;
    sect_wr->cnt -= grp_cnt;
    sect_wr->vsn0 += grp_cnt;

    // Return success.
    return 0;
}
#endif // INC_SECT_FTL

// flush_pending_writes: Write any pending consecutive writes to flash
//
//      Inputs: ftl = pointer to FTL control block
//              page_wr = pointer to structure holding VPN, PN, count,
//                        and buffer pointer
//
//     Returns: 0 on success, -1 on failure
//
static int flush_pending_writes(FTLN ftl, PageWr* page_wr) {
    ui32 end, b = page_wr->ppn0 / ftl->pgs_per_blk;

#if INC_ELIST
    // If list of erased blocks/wear counts exists, erase it now.
    if (ftl->elist_blk != (ui32)-1)
        if (FtlnEraseBlk(ftl, ftl->elist_blk))
            return -1;
#endif

    // Issue driver multi-page write request. Return -1 if error.
    if (ftl->write_pages(ftl->start_pn + page_wr->ppn0, page_wr->cnt, page_wr->buf, ftl->spare_buf,
                         ftl->ndm))
        return FtlnFatErr(ftl);
    ftl->stats.write_page += page_wr->cnt;

#if INC_FTL_PAGE_CACHE
    // If virtual pages cache, update any entry that contains any of the
    // successfully written pages.
    if (ftl->vol_cache)
        ftlvcUpdate(ftl->vol_cache, page_wr->vpn0, page_wr->cnt, page_wr->buf, ftl->page_size);
#endif

    // Loop over all written pages to update mappings.
    end = page_wr->ppn0 + page_wr->cnt;
    for (; page_wr->ppn0 < end; ++page_wr->ppn0, ++page_wr->vpn0) {
        ui32 old_pn;

        // Retrieve old mapping for current page. Return -1 if error.
        if (FtlnMapGetPpn(ftl, page_wr->vpn0, &old_pn) < 0)
            return -1;

        // If mapping exists, decrement number of used in old block.
        if (old_pn != (ui32)-1)
            FtlnDecUsed(ftl, old_pn, page_wr->vpn0);

        // Increment number of used in new block.
        PfAssert(!IS_FREE(ftl->bdata[b]) && !IS_MAP_BLK(ftl->bdata[b]));
        INC_USED(ftl->bdata[b]);

        // Update mapping for current virtual page. Return -1 if error.
        if (FtlnMapSetPpn(ftl, page_wr->vpn0, page_wr->ppn0))
            return -1;
        PfAssert(ftl->num_free_blks >= FTLN_MIN_FREE_BLKS);
    }

    // Return success.
    return 0;
}

// write_sectors: Write virtual sectors
//
//      Inputs: ftl = pointer to FTL control block
//              vsn = start virtual sector to write
//              count = number of consecutive sectors to write
//              buf = buffer to hold written sectors contents
//
//     Returns: 0 on success, -1 on error
//
static int write_sectors(FTLN ftl, ui32 vsn, ui32 count, const ui8* buf) {
    SectWr sect_wr;
    PageWr page_wr;

    // Set errno and return -1 if fatal I/O error occurred.
    if (ftl->flags & FTLN_FATAL_ERR)
        return FsError(EIO);

    // Initialize structures for staging sector and page writes.
    sect_wr.buf = buf;
    sect_wr.cnt = count;
    sect_wr.vsn0 = vsn;
    page_wr.buf = NULL;
    page_wr.cnt = 0;
    page_wr.ppn0 = page_wr.vpn0 = (ui32)-1;

#if INC_SECT_FTL
    // If sector is not page aligned, perform partial first page write.
    if (sect_wr.vsn0 % ftl->sects_per_page)
        if (partial_page_write(ftl, &sect_wr))
            return -1;

    // Check if there are any whole pages to write.
    if (sect_wr.cnt >= ftl->sects_per_page)
#endif
    {
        int need_recycle;
        ui8* spare = ftl->spare_buf;

        // Check if recycles are needed for one page write.
        need_recycle = FtlnRecNeeded(ftl, 1);

        // Loop while there are whole pages to write.
        do {
            ui32 pn, vpn, wc;

            // If needed, recycle blocks until at least one page is free.
            if (need_recycle)
                if (FtlnRecCheck(ftl, 1))
                    return -1;

            // Allocate next free volume page. Return -1 if error.
            pn = next_free_vpg(ftl);
            if (pn == (ui32)-1)
                return pn;

            // Compute virtual page number associated with sector and ensure
            // sector is first in page.
            vpn = sect_wr.vsn0 / ftl->sects_per_page;
            PfAssert(sect_wr.vsn0 % ftl->sects_per_page == 0);

            // If no pending writes, start new sequence. Else add to it.
            if (page_wr.vpn0 == (ui32)-1) {
                page_wr.vpn0 = vpn;
                page_wr.buf = sect_wr.buf;
                page_wr.ppn0 = pn;
                page_wr.cnt = 1;
                spare = ftl->spare_buf;
            } else
                ++page_wr.cnt;

            // Set the spare area VPN, BC, and block WC for this page.
            memset(spare, 0xFF, ftl->eb_size);
            SET_SA_VPN(vpn, spare);
            wc = ftl->high_wc - ftl->blk_wc_lag[pn / ftl->pgs_per_blk];
            SET_SA_WC(wc, spare);

            // Check if physical page number is at block boundary or writing
            // one more page than staged would trigger a recycle.
            need_recycle = FtlnRecNeeded(ftl, page_wr.cnt + 1);
            if ((ftl->free_vpn == (ui32)-1) || need_recycle) {
                // If any, flush currently staged pages and reset the staging.
                if (page_wr.vpn0 != (ui32)-1) {
                    if (flush_pending_writes(ftl, &page_wr))
                        return -1;
                    page_wr.vpn0 = (ui32)-1;
                }

                // Invoke recycles to prepare for the next page write.
                need_recycle = TRUE;
            }

            // Advance input parameters and spare buffer pointer.
            sect_wr.buf += ftl->page_size;
            sect_wr.cnt -= ftl->sects_per_page;
            sect_wr.vsn0 += ftl->sects_per_page;
            spare += ftl->eb_size;
        } while (sect_wr.cnt >= ftl->sects_per_page);

        // If there are any, flush pending writes.
        if (page_wr.vpn0 != (ui32)-1) {
            if (FtlnRecCheck(ftl, page_wr.cnt))
                return -1;
            if (flush_pending_writes(ftl, &page_wr))
                return -1;
        }
    }

#if INC_SECT_FTL
    // If still unwritten sectors, do partial write of last page.
    if (sect_wr.cnt)
        if (partial_page_write(ftl, &sect_wr))
            return -1;
#endif

    // Return success.
    return 0;
}

// Global Function Definitions

// FtlnRecNeeded: Determine if dirty flash pages need to be reclaimed
//
//      Inputs: ftl = pointer to FTL control block
//              wr_cnt = the number and type of pending page writes,
//                       in addition to dirty map cache pages:
//                       < 0 -> one map page
//                       > 0 -> wr_cnt number of volume pages
//                       = 0 -> no additional (besides map cache)
//
int FtlnRecNeeded(CFTLN ftl, int wr_cnt) {
    int free_pgs, need, mblks_req, vblks_req;

    // Return TRUE if some block is at read count max.
    if (ftl->max_rc_blk != (ui32)-1)
        return TRUE;

    // Return TRUE if in powerfail recovery of an interrupted recycle.
    if (ftl->num_free_blks < FTLN_MIN_FREE_BLKS)
        return TRUE;

    // If free map list can hold all currently dirty map cache pages and
    // those potentially made dirty by the vol page write, no more map
    // blocks are needed. Otherwise convert needed map pages to blocks.
    free_pgs = free_map_list_pgs(ftl);
    if (wr_cnt < 0)
        need = 1;  // writing one map page
    else
        need = wr_cnt;  // each volume page can update one map page
    need += ftl->map_cache->num_dirty;
    if (free_pgs >= need)
        mblks_req = 0;
    else {
        ui32 avail_blk_pgs = ftl->pgs_per_blk;

#if INC_FTL_NDM_MLC
        if (ftl->type == NDM_MLC)
            avail_blk_pgs /= 2;
#endif
        mblks_req = (need - free_pgs + avail_blk_pgs - 1) / avail_blk_pgs;
    }

    // If free volume list can hold all requested write pages, no more
    // volume blocks are needed. Otherwise convert needed pages to blocks
    free_pgs = free_vol_list_pgs(ftl);
    if (wr_cnt <= free_pgs)
        vblks_req = 0;  // no new volume blocks needed
    else
        vblks_req = (wr_cnt - free_pgs + ftl->pgs_per_blk - 1) / ftl->pgs_per_blk;

    // Need recycle if number of required blocks is more than is free.
    return (ui32)(mblks_req + vblks_req + FTLN_MIN_FREE_BLKS) > ftl->num_free_blks;
}

// FtlnRecycleMapBlk: Recycle one map block
//
//      Inputs: ftl = pointer to FTL control block
//              recycle_b = block to be recycled
//
//     Returns: 0 on success, -1 on error
//
int FtlnRecycleMapBlk(FTLN ftl, ui32 recycle_b) {
    ui32 i, pn;

#if FTLN_DEBUG > 1
    printf("FtlnRecycleMapBlk: block# %u\n", recycle_b);
#endif

    // Transfer all used pages from recycle block to free block.
    pn = recycle_b * ftl->pgs_per_blk;
    for (i = 0; NUM_USED(ftl->bdata[recycle_b]); ++pn, ++i) {
        int rc;
        ui32 mpn;
        void* buf;

        // Error if looped over block and not enough valid used pages.
        if (i >= ftl->pgs_per_blk)
            return FtlnFatErr(ftl);

#if INC_FTL_NDM_MLC
        // If MLC NAND and not first page on block, skip pages whose pair
        // is at a lower offset, to not corrupt them by a failed write.
        if (ftl->type == NDM_MLC && i)
            if (ftl->pair_offset(i, ftl->ndm) < i)
                continue;
#endif

        // Read page's spare area. Return -1 if fatal I/O error.
        ++ftl->stats.read_spare;
        rc = ftl->read_spare(ftl->start_pn + pn, ftl->spare_buf, ftl->ndm);
        if (rc == -2)
            return FtlnFatErr(ftl);

        // Skip page if ECC error on spare read.
        if (rc < 0)
            continue;

        // Get map page number from page's spare area.
        mpn = GET_SA_VPN(ftl->spare_buf);

        // Skip page if meta page or physical page mapping is outdated.
        if (mpn >= ftl->num_map_pgs - 1 || ftl->mpns[mpn] != pn)
            continue;

        // Get pointer to its data if page-to-be-written is in cache.
        buf = ftlmcInCache(ftl->map_cache, mpn);

        // Write page to new flash block. Return -1 if error.
        if (wr_map_page(ftl, mpn, buf))
            return -1;
    }

    // Erase recycled map block. Return -1 if error.
    if (FtlnEraseBlk(ftl, recycle_b))
        return -1;

    // If this is last block at RC limit, flag that none are at limit.
    if (ftl->max_rc_blk == recycle_b)
        ftl->max_rc_blk = (ui32)-1;

    // Return success.
    return 0;
}

#if INC_FTL_PAGE_CACHE
//   FtlnVpnWr: Cache function to write a volume page
//
//      Inputs: c_e = cache entry with volume page number/content
//              vol_ptr = FTL handle
//
//     Returns: 0 on success, -1 on failure
//
int FtlnVpnWr(FcEntry* c_e, int unused, void* vol_ptr) {
    FTLN ftl = vol_ptr;

    return write_sectors(ftl, c_e->sect_num * ftl->sects_per_page, ftl->sects_per_page, c_e->data);
} //lint !e818
#endif

//  FtlnMetaWr: Write FTL meta information page
//
//      Inputs: ftl = pointer to FTL control block
//              type = metapage type
//
//        Note: The caller should initialize all but the first 8 bytes
//              of 'main_buf' before calling this routine.
//
//     Returns: 0 on success, -1 on error
//
int FtlnMetaWr(FTLN ftl, ui32 type) {
    ui32 mpn = ftl->num_map_pgs - 1;

    // Write metapage version number and type.
    WR32_LE(FTLN_META_VER1, &ftl->main_buf[FTLN_META_VER_LOC]);
    WR32_LE(type, &ftl->main_buf[FTLN_META_TYP_LOC]);

    // Issue meta page write.
    return wr_map_page(ftl, mpn, ftl->main_buf);
}

// FtlnRecCheck: Prepare to write page(s) by reclaiming dirty blocks
//               in advance to (re)establish the reserved number of
//               free blocks
//
//      Inputs: ftl = pointer to FTL control block
//              wr_cnt = the number and type of page writes to prepare
//                       for, in addition to dirty map cache pages:
//                       < 0 -> one map page
//                       > 0 -> wr_cnt number of volume pages
//                       = 0 -> no additional (besides map cache)
//
//     Returns: 0 on success, -1 on error
//
int FtlnRecCheck(FTLN ftl, int wr_cnt) {
    ui32 count = 0;

    // Set errno and return -1 if fatal I/O error occurred.
    if (ftl->flags & FTLN_FATAL_ERR)
        return FsError(EIO);

    // Keep looping until enough pages are free.
    while (FtlnRecNeeded(ftl, wr_cnt)) {
#if FTLN_DEBUG > 1
        printf(
            "\nrec begin: free vpn = %5u (%3u), free mpn = %5u (%3u) "
            "free blocks = %2u\n",
            ftl->free_vpn, free_vol_list_pgs(ftl), ftl->free_mpn, free_map_list_pgs(ftl),
            ftl->num_free_blks);
#endif

        // Ensure we haven't recycled too many times.
        PfAssert(count <= 2 * ftl->num_blks);
        if (count > 2 * ftl->num_blks) {
#if FTLN_DEBUG
            printf("FTL NDM too many consec recycles = %u\n", count);
            FtlnBlkStats(ftl);
#endif
            return FsError(ENOSPC);
        }

        // Perform one recycle operation. Return -1 if error.
        if (recycle(ftl))
            return -1;
        ++count;

#if FTLN_DEBUG_RECYCLES
        // If enabled, list the number free and the state of each block.
        if (FtlnShow) {
            printf("num_free=%d\n", ftl->num_free_blks);
            show_blks(ftl);
        }
#endif

        // If deferring wear dominated block selects, decrement deferment.
        if (ftl->deferment)
            --ftl->deferment;

#if FTLN_DEBUG_RECYCLES
        if (MaxCnt < count)
            MaxCnt = count;
        if (count > ftl->num_blks)
            FtlnShow = TRUE;
#endif
    }

    // Return success.
    return 0;
}

// FtlnMapGetPpn: Map virtual page number to its physical page number
//
//      Inputs: ftl = pointer to FTL control block
//              vpn = VPN to look up
//      Output: *pnp = physical page number or -1 if unmapped
//
//        Note: By causing a map cache page flush, this routine can
//              consume one free page
//
//     Returns: 0 on success, -1 on failure
//
int FtlnMapGetPpn(CFTLN ftl, ui32 vpn, ui32* pnp) {
    ui32 mpn, ppn;
    ui8* maddr;
    int unmapped;

    // Determine map page to use.
    PfAssert(vpn <= ftl->num_vpages);
    mpn = vpn / ftl->mappings_per_mpg;

    // Retrieve map page via cache. Return -1 if I/O error.
    maddr = ftlmcGetPage(ftl->map_cache, mpn, &unmapped);
    if (maddr == NULL)
        return -1;

    // If page is unmapped, set page number to -1.
    if (unmapped)
        ppn = (ui32)-1;

    // Else get physical page number from map. Equals -1 if unmapped.
    else {
        // Calculate address of VPN's entry in map page and read it.
        maddr += (vpn % ftl->mappings_per_mpg) * FTLN_PN_SZ;
        ppn = GET_MAP_PPN(maddr);

        // If physical page number is too big, it is unmapped.
        if (ppn >= ftl->num_pages)
            ppn = (ui32)-1;

#if FS_ASSERT
        // Else verify that it lies in a volume block.
        else {
            ui32 b = ppn / ftl->pgs_per_blk;

            PfAssert(!IS_MAP_BLK(ftl->bdata[b]) && !IS_FREE(ftl->bdata[b]));
        }
#endif
    }

    // Output physical page for VPN and return success.
    *pnp = ppn;
    return 0;
}

// FtlnMapSetPpn: Set new physical page number in given VSN's map page
//
//      Inputs: ftl = pointer to FTL control block
//              vpn = VPN to create new mapping for
//              ppn = new physical location for VPN or -1 if unmapped
//
//        Note: By causing a map cache page flush, this routine can
//              consume one free page
//
//     Returns: 0 on success, -1 on failure
//
int FtlnMapSetPpn(CFTLN ftl, ui32 vpn, ui32 ppn) {
    ui32 mpn;
    ui8* maddr;

    // Determine map page to use.
    PfAssert(vpn <= ftl->num_vpages);
    mpn = vpn / ftl->mappings_per_mpg;

    // Retrieve map page contents via cache, marking it dirty if clean.
    maddr = ftlmcGetPage(ftl->map_cache, mpn, NULL);
    if (maddr == NULL)
        return -1;

    // Calculate address of VSN's entry in map page.
    maddr += (vpn % ftl->mappings_per_mpg) * FTLN_PN_SZ;

    // Set physical page number for VPN and return success.
    SET_MAP_PPN(maddr, ppn);
    return 0;
}

//  FtlnVclean: Perform vclean() on FTL volume
//
//       Input: ftl = pointer to FTL control block
//
//     Returns: 0 if no more vclean needed, 1 if future vclean needed,
//              -1 on error
//
int FtlnVclean(FTLN ftl) {
    ui32 b;

    // Set errno and return -1 if fatal I/O error occurred.
    if (ftl->flags & FTLN_FATAL_ERR)
        return FsError(EIO);

    // Check if the dirty pages garbage level is > 9.
    if (FtlnGarbLvl(ftl) >= 10) {
        // Perform one recycle operation. Return -1 if error.
        if (recycle(ftl))
            return -1;

        // Return '1' so that vclean() is called again.
        return 1;
    }

    // Look for a block that is free, but not erased.
    for (b = 0; b < ftl->num_blks; ++b) {
        if (IS_FREE(ftl->bdata[b]) && !IS_ERASED(ftl->bdata[b])) {
            // Erase block. Return -1 if error.
            if (FtlnEraseBlk(ftl, b))
                return -1;

            // Return '1' so that vclean() is called again.
            return 1;
        }
    }

    // Nothing to do, return '0'.
    return 0;
}

// FtlnWrSects: Write count number of volume sectors to flash
//
//      Inputs: buf = place that holds data bytes for sectors
//              sect = first sector to write to
//              count = number of sectors to write
//              vol = FTL handle
//
//     Returns: 0 on success, -1 on error
//
int FtlnWrSects(const void* buf, ui32 sect, int count, void* vol) {
    FTLN ftl = vol;
    int status;

    // Ensure request is within volume's range of provided sectors.
    if (sect + count > ftl->num_vsects)
        return FsError(ENOSPC);

    // If no sectors to write, return success.
    if (count == 0)
        return 0;

#if INC_FAT_MBR
    // Ensure all cluster requests are page aligned.
    if (sect >= ftl->frst_clust_sect)
        sect += ftl->clust_off;

    // When first sector is written, check if it's a master boot one and
    // update the boot sector value for the volume then.
    if (sect == 0) {
        FATPartition part;

        if (FatGetPartitions(buf, &part, 1) == 1) {
            int diff = part.first_sect - ftl->vol_frst_sect;

            ftl->vol_frst_sect = part.first_sect;
            ftl->num_vsects -= diff;
        }
    }
#endif

    // Write sectors. Return -1 if error.
    status = write_sectors(ftl, sect, count, buf);
    if (status)
        return -1;

#if INC_FAT_MBR
    // If FAT boot sector has just been written, set frst_clust_sect.
    if (FLAG_IS_SET(ftl->flags, FTLN_FAT_VOL) && sect == ftl->vol_frst_sect)
        status = FtlnSetClustSect1(ftl, buf, buf == ftl->main_buf);
#endif

    // Return status.
    return status;
} //lint !e429

//   FtlnMapWr: Write a map page to flash - used by map page cache
//
//      Inputs: vol = FTL handle
//              mpn = map page to write
//              buf = pointer to contents of map page
//
//     Returns: 0 on success, -1 on error
//
int FtlnMapWr(void* vol, ui32 mpn, void* buf) {
    FTLN ftl = vol;

    // Sanity check that mpn is valid and not the meta page.
    PfAssert(mpn < ftl->num_map_pgs - 1);

    // Write map page to flash.
    return wr_map_page(ftl, mpn, buf);
}

// FtlnGarbLvl: Calculate the volume garbage level
//
//       Input: ftl = pointer to FTL control block
//
//     Returns: Calculated volume garbage level
//
ui32 FtlnGarbLvl(CFTLN ftl) {
    ui32 b, free_pages, used_pages = 0;

    // Count the number of used pages.
    for (b = 0; b < ftl->num_blks; ++b)
        if (!IS_FREE(ftl->bdata[b]))
            used_pages += NUM_USED(ftl->bdata[b]);

    // Count the number of free pages.
    free_pages = ftl->num_free_blks * ftl->pgs_per_blk;
    free_pages += free_vol_list_pgs(ftl);
    free_pages += free_map_list_pgs(ftl);

    // Garbage level is given by the following formula:
    //                        F
    //     GL = 100 x (1 -  -----)
    //                      T - U
    // where:
    //   F  = # of free pages in volume
    //   T  = # of pages in volume
    //   U  = # of used pages in volume
    // The result is a number in [0, 100) indicating percentage
    // of space that is dirty from the total available.
    return 100 - (100 * free_pages) / (ftl->num_pages - used_pages);
}

#if FTLN_DEBUG_RECYCLES && FTLN_DEBUG_PTR
// FtlnShowBlks: Display block WC, selection priority, etc
//
void FtlnShowBlks(void) {
    semPend(FileSysSem, WAIT_FOREVER);
    show_blks(FtlnDebug);
    semPostBin(FileSysSem);
}
#endif

#endif // INC_FTL_NDM

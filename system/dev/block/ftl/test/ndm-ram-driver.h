// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <inttypes.h>

#include <fbl/array.h>
#include <lib/ftl/ndm-driver.h>
#include <zircon/types.h>

// How often to inject errors.
constexpr int kEccErrorInterval = 900;
constexpr int kBadBlockInterval = 50;

// Ram-backed driver for testing purposes.
class NdmRamDriver : public ftl::NdmBaseDriver {
  public:
    NdmRamDriver(const ftl::VolumeOptions& options) : options_(options) {}
    ~NdmRamDriver() final {}

    // NdmDriver interface:
    const char* Init() final;
    const char* Attach(const ftl::Volume* ftl_volume) final;
    bool Detach() final;
    int NandRead(uint32_t start_page, uint32_t page_count, void* page_buffer, void* oob_buffer) final;
    int NandWrite(uint32_t start_page, uint32_t page_count, const void* page_buffer,
                  const void* oob_buffer) final;
    int NandErase(uint32_t page_num) final;
    int IsBadBlock(uint32_t page_num) final;
    bool IsEmptyPage(uint32_t page_num, const uint8_t* data, const uint8_t* spare) final;

  private:
    // Reads or Writes a single page.
    int ReadPage(uint32_t page_num, uint8_t* data, uint8_t* spare);
    int WritePage(uint32_t page_num, const uint8_t* data, const uint8_t* spare);

    // Returns true for a freshly minted bad block.
    bool SimulateBadBlock(uint32_t page_num);

    // Access the main data and spare area for a given page.
    uint8_t* MainData(uint32_t page_num);
    uint8_t* SpareData(uint32_t page_num);

    // Access flags for a given page.
    bool Written(uint32_t page_num);
    bool FailEcc(uint32_t page_num);
    bool BadBlock(uint32_t page_num);
    void SetWritten(uint32_t page_num, bool value);
    void SetFailEcc(uint32_t page_num, bool value);
    void SetBadBlock(uint32_t page_num, bool value);

    uint32_t PagesPerBlock() const;

    fbl::Array<uint8_t> volume_;
    fbl::Array<uint8_t> flags_;
    ftl::VolumeOptions options_;
    int ecc_error_interval_ = 0;  // Controls simulation of ECC errors.
    int bad_block_interval_ = 0;  // Controls simulation of bad blocks.
    uint32_t num_bad_blocks_ = 0;
};

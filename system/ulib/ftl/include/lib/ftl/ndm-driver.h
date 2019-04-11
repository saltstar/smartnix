// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <inttypes.h>

struct ndm;

namespace ftl {

class Volume;

// Return values expected by NDM from the nand driver.
// See ndm-man-20150.pdf for the complete low level API specification:
constexpr int kNdmOk = 0;
constexpr int kNdmError = -1;
constexpr int kNdmUncorrectableEcc = -1;
constexpr int kNdmFatalError = -2;
constexpr int kNdmUnsafeEcc = 1;
constexpr int kTrue = 1;
constexpr int kFalse = 0;

// Options for a device to be created. All sizes are in bytes.
struct VolumeOptions {
    uint32_t num_blocks;
    uint32_t max_bad_blocks;
    uint32_t block_size;
    uint32_t page_size;
    uint32_t eb_size;  // Extra bytes, a.k.a. OOB.
    uint32_t flags;
};

// Encapsulates the lower layer TargetFtl-Ndm driver.
class NdmDriver {
  public:
    virtual ~NdmDriver() {}

    // Performs driver initialization. Returns an error string, or nullptr on
    // success.
    virtual const char* Init() = 0;

    // Creates a new volume. Note that multiple volumes are not supported.
    // |ftl_volume| (if provided) will be notified with the volume details.
    // Returns an error string, or nullptr on success.
    virtual const char* Attach(const Volume* ftl_volume) = 0;

    // Destroy the volume created with Attach(). Returns true on success.
    virtual bool Detach() = 0;

    // Reads |page_count| pages starting at |start_page|, placing the results on
    // |page_buffer| and |oob_buffer|. Either pointer can be nullptr if that
    // part is not desired.
    // Returns kNdmOk, kNdmUncorrectableEcc, kNdmFatalError or kNdmUnsafeEcc.
    virtual int NandRead(uint32_t start_page, uint32_t page_count, void* page_buffer,
                         void* oob_buffer) = 0;

    // Writes |page_count| pages starting at |start_page|, using the data from
    // |page_buffer| and |oob_buffer|.
    // Returns kNdmOk, kNdmError or kNdmFatalError. kNdmError triggers marking
    // the block as bad.
    virtual int NandWrite(uint32_t start_page, uint32_t page_count, const void* page_buffer,
                          const void* oob_buffer) = 0;

    // Erases the block containing |page_num|.
    // Returns kNdmOk or kNdmError. kNdmError triggers marking the block as bad.
    virtual int NandErase(uint32_t page_num) = 0;

    // Returns whether the block containing |page_num| was factory-marked as bad.
    // Returns kTrue, kFalse or kNdmError.
    virtual int IsBadBlock(uint32_t page_num) = 0;

    // Returns whether a given page is empty or not. |data| and |spare| store
    // the contents of the page.
    virtual bool IsEmptyPage(uint32_t page_num, const uint8_t* data, const uint8_t* spare) = 0;
};

// Base functionality for a driver implementation.
class NdmBaseDriver : public NdmDriver {
  public:
    NdmBaseDriver() {}
    virtual ~NdmBaseDriver();

    // Creates the underlying NDM volume, with the provided parameters.
    const char* CreateNdmVolume(const Volume* ftl_volume, const VolumeOptions& options);

    // Deletes the underlying NDM volume.
    bool RemoveNdmVolume();

    // Inspects |data_len| bytes from |data| and |spare_len| bytes from |spare|
    // looking for a typical empty (erased) page. Returns true if all bits are 1.
    bool IsEmptyPageImpl(const uint8_t* data, uint32_t data_len, const uint8_t* spare,
                         uint32_t spare_len) const;

  private:
    ndm* ndm_ = nullptr;
};

// Performs global module initialization. This is exposed to support unit tests,
// and while calling this function multiple times is supported, racing calls are
// not (or more generally, calling this from multiple threads).
// If multiple simultaneous tests from the same test instance ever becomes a
// thing, this should be called from testing::Environment and not from each
// test case.
bool InitModules();

}  // namespace ftl

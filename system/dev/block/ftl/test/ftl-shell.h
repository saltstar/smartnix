// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <lib/ftl/volume.h>

#include "ndm-ram-driver.h"

// Placeholder for an FTL driver, for testing purposes. The implementation uses
// an NdmRamDriver at the lower layer interface.
class FtlShell : public ftl::FtlInstance {
  public:
    FtlShell() : volume_(this) {}
    virtual ~FtlShell() {}

    bool Init(const ftl::VolumeOptions& options);
    bool ReAttach();

    ftl::Volume* volume() { return &volume_; }
    uint32_t page_size() const { return page_size_; }
    uint32_t num_pages() const { return num_pages_; }

    // FtlInstance interface.
    bool OnVolumeAdded(uint32_t page_size, uint32_t num_pages) final;

  private:
    ftl::VolumeImpl volume_;
    uint32_t page_size_ = 0;
    uint32_t num_pages_ = 0;
};

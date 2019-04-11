// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZIRCON_SYSTEM_DEV_SYSMEM_SYSMEM_DEVICE_H_
#define ZIRCON_SYSTEM_DEV_SYSMEM_SYSMEM_DEVICE_H_

#include <ddk/binding.h>
#include <ddk/debug.h>
#include <ddk/device.h>
#include <ddk/driver.h>
#include <ddk/protocol/platform/device.h>
#include <ddk/protocol/sysmem.h>
#include <fbl/unique_ptr.h>
#include <fbl/vector.h>
#include <fuchsia/sysmem/c/fidl.h>
#include <lib/zx/bti.h>
#include <region-alloc/region-alloc.h>

#include "protected_memory_allocator.h"

#include <limits>
#include <map>

class Driver;
class BufferCollectionToken;
class Device {
public:
    Device(zx_device_t* parent_device, Driver* parent_driver);

    zx_status_t Bind();

    //
    // The rest of the methods are only valid to call after Bind().
    //

    zx_status_t Connect(zx_handle_t allocator_request);

    const zx::bti& bti();

    uint32_t pdev_device_info_vid();

    uint32_t pdev_device_info_pid();

    // Track/untrack the token by the koid of the server end of its FIDL
    // channel.  TrackToken() is only allowed after token->SerServerKoid().
    // UntrackToken() is allowed even if there was never a
    // token->SetServerKoid() (in which case it's a nop).
    //
    // While tracked, a token can be found with FindTokenByServerChannelKoid().
    void TrackToken(BufferCollectionToken* token);
    void UntrackToken(BufferCollectionToken* token);

    // Find the BufferCollectionToken (if any) by the koid of the server end of
    // its FIDL channel.
    BufferCollectionToken* FindTokenByServerChannelKoid(
        zx_koid_t token_server_koid);

    ProtectedMemoryAllocator* protected_allocator() { return protected_allocator_.get(); }

private:
    zx_device_t* parent_device_ = nullptr;
    Driver* parent_driver_ = nullptr;

    pdev_protocol_t pdev_{};
    zx::bti bti_;

    zx_device_t* device_ = nullptr;

    // Initialize these to a value that won't be mistaken for a real vid or pid.
    uint32_t pdev_device_info_vid_ = std::numeric_limits<uint32_t>::max();
    uint32_t pdev_device_info_pid_ = std::numeric_limits<uint32_t>::max();

    // In-proc sysmem interface.  Essentially an in-proc version of
    // fuchsia.sysmem.DriverConnector.
    sysmem_protocol_t in_proc_sysmem_protocol_;

    // This map allows us to look up the BufferCollectionToken by the koid of
    // the server end of a BufferCollectionToken channel.
    std::map<zx_koid_t, BufferCollectionToken*> tokens_by_koid_;

    fbl::unique_ptr<ProtectedMemoryAllocator> protected_allocator_;
};

#endif // ZIRCON_SYSTEM_DEV_SYSMEM_SYSMEM_DEVICE_H_

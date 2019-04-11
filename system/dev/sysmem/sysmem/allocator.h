// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZIRCON_SYSTEM_DEV_SYSMEM_SYSMEM_ALLOCATOR_H_
#define ZIRCON_SYSTEM_DEV_SYSMEM_SYSMEM_ALLOCATOR_H_

#include "device.h"
#include "logging.h"

#include "fuchsia/sysmem/c/fidl.h"
#include "lib/fidl-async-2/fidl_server.h"
#include "lib/fidl-async-2/simple_binding.h"

// An instance of this class serves an Allocator2 connection.  The lifetime of
// the instance is 1:1 with the Allocator2 channel.
//
// Because Allocator is essentially self-contained and handling the server end
// of a channel, most of Allocator is private.
class Allocator : public FidlServer<
                      Allocator,
                      SimpleBinding<Allocator, fuchsia_sysmem_Allocator2_ops_t,
                                    fuchsia_sysmem_Allocator2_dispatch>,
                      vLog> {
public:
    // Public for std::unique_ptr<Allocator>:
    ~Allocator();

private:
    friend class FidlServer;
    Allocator(Device* parent_device);

    static const fuchsia_sysmem_Allocator2_ops_t kOps;

    zx_status_t
    AllocateNonSharedCollection(zx_handle_t buffer_collection_request_param);
    zx_status_t AllocateSharedCollection(zx_handle_t token_request);
    zx_status_t BindSharedCollection(zx_handle_t token,
                                     zx_handle_t buffer_collection);

    Device* parent_device_ = nullptr;
};

#endif // ZIRCON_SYSTEM_DEV_SYSMEM_SYSMEM_ALLOCATOR_H_

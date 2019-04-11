// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZIRCON_SYSTEM_DEV_SYSMEM_SYSMEM_BUFFER_COLLECTION_H_
#define ZIRCON_SYSTEM_DEV_SYSMEM_SYSMEM_BUFFER_COLLECTION_H_

#include "logging.h"

#include <fuchsia/sysmem/c/fidl.h>
#include <lib/fidl-async-2/fidl_server.h>
#include <lib/fidl-async-2/fidl_struct.h>
#include <lib/fidl-async-2/simple_binding.h>
#include <lib/fidl/internal.h>

#include <list>

extern const fidl_type_t fuchsia_sysmem_BufferCollectionConstraintsTable;
extern const fidl_type_t fuchsia_sysmem_BufferCollectionInfo_2Table;

class LogicalBufferCollection;
class BufferCollection
    : public FidlServer<
          BufferCollection,
          SimpleBinding<BufferCollection, fuchsia_sysmem_BufferCollection_ops_t,
                        fuchsia_sysmem_BufferCollection_dispatch>,
          vLog> {
public:
    ~BufferCollection();

    //
    // BufferCollection interface methods
    //

    zx_status_t SetEventSink(zx_handle_t buffer_collection_events_client);
    zx_status_t Sync(fidl_txn_t* txn);
    zx_status_t SetConstraints(
        bool has_constraints,
        const fuchsia_sysmem_BufferCollectionConstraints* constraints);
    zx_status_t WaitForBuffersAllocated(fidl_txn_t* txn);
    zx_status_t CloseSingleBuffer(uint64_t buffer_index);
    zx_status_t AllocateSingleBuffer(uint64_t buffer_index);
    zx_status_t WaitForSingleBufferAllocated(uint64_t buffer_index,
                                             fidl_txn_t* txn);
    zx_status_t CheckSingleBufferAllocated(uint64_t buffer_index);
    zx_status_t Close();

    //
    // LogicalBufferCollection uses these:
    //

    using BufferCollectionInfo =
        FidlStruct<fuchsia_sysmem_BufferCollectionInfo_2,
                   &fuchsia_sysmem_BufferCollectionInfo_2Table>;

    void OnBuffersAllocated();

    bool set_constraints_seen();

    // nullptr means Null constraints
    const fuchsia_sysmem_BufferCollectionConstraints* constraints();

    LogicalBufferCollection* parent();

    fbl::RefPtr<LogicalBufferCollection> parent_shared();

    bool is_done();

private:
    friend class FidlServer;
    using Constraints =
        FidlStruct<fuchsia_sysmem_BufferCollectionConstraints,
                   &fuchsia_sysmem_BufferCollectionConstraintsTable>;

    BufferCollection(fbl::RefPtr<LogicalBufferCollection> parent);

    // The rights attenuation mask driven by usage, so that read-only usage
    // doesn't get write, etc.
    uint32_t GetUsageBasedRightsAttenuation();

    uint32_t GetClientVmoRights();
    void MaybeCompleteWaitForBuffersAllocated();
    BufferCollectionInfo BufferCollectionInfoClone(
        const fuchsia_sysmem_BufferCollectionInfo_2* buffer_collection_info);

    static const fuchsia_sysmem_BufferCollection_ops_t kOps;

    fbl::RefPtr<LogicalBufferCollection> parent_;

    // Client end of a BufferCollectionEvents channel, for the local server to
    // send events to the remote client.  All of the messages in this interface
    // are one-way with no response, so sending an event doesn't block the
    // server thread.
    //
    // This may remain non-set if SetEventSink() is never used by a client.  A
    // client may send SetEventSink() up to once.
    //
    // For example:
    // fuchsia_sysmem_BufferCollectionEventsOnBuffersAllocated(
    //     events_.get(), ...);
    zx::channel events_;

    bool set_constraints_seen_ = false;
    Constraints constraints_{Constraints::Null};

    // The rights attenuation mask driven by BufferCollectionToken::Duplicate()
    // rights_attenuation_mask parameter(s) as the token is duplicated,
    // potentially via multiple participants.
    uint32_t client_rights_attenuation_mask_ =
        std::numeric_limits<uint32_t>::max();

    std::list<std::unique_ptr<BindingType::Txn>>
        pending_wait_for_buffers_allocated_;

    bool is_done_ = false;
};

#endif // ZIRCON_SYSTEM_DEV_SYSMEM_SYSMEM_BUFFER_COLLECTION_H_

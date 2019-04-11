// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "logical_buffer_collection.h"

#include "buffer_collection.h"
#include "buffer_collection_token.h"
#include "koid_util.h"
#include "usage_pixel_format_cost.h"

#include <lib/image-format/image_format.h>
#include <limits.h> // PAGE_SIZE
#include <zircon/assert.h>

namespace {

// Sysmem is creating the VMOs, so sysmem can have all the rights and just not
// mis-use any rights.  Remove ZX_RIGHT_EXECUTE though.
const uint32_t kSysmemVmoRights = ZX_DEFAULT_VMO_RIGHTS & ~ZX_RIGHT_EXECUTE;
// 1 GiB cap for now.
const uint64_t kMaxTotalSizeBytesPerCollection = 1ull * 1024 * 1024 * 1024;
// 256 MiB cap for now.
const uint64_t kMaxSizeBytesPerBuffer = 256ull * 1024 * 1024;

template <typename T> bool IsNonZeroPowerOf2(T value) {
    if (!value) {
        return false;
    }
    if (value & (value - 1)) {
        return false;
    }
    return true;
}

// TODO(dustingreen): Switch to FIDL C++ generated code (preferred) and remove
// this, or fully implement something like this for all fields that need 0 to
// imply a default value that isn't 0.
template <typename T> T FieldDefault1(T value) {
    if (value == 0) {
        return 1;
    }
    return value;
}

template <typename T> T AlignUp(T value, T divisor) {
    return (value + divisor - 1) / divisor * divisor;
}

bool IsCpuUsage(const fuchsia_sysmem_BufferUsage& usage) {
    return usage.cpu != 0;
}

} // namespace

// static
void LogicalBufferCollection::Create(
    zx::channel buffer_collection_token_request, Device* parent_device) {
    fbl::RefPtr<LogicalBufferCollection> logical_buffer_collection =
        fbl::AdoptRef<LogicalBufferCollection>(
            new LogicalBufferCollection(parent_device));
    // The existence of a channel-owned BufferCollectionToken adds a
    // fbl::RefPtr<> ref to LogicalBufferCollection.
    LogInfo("LogicalBufferCollection::Create()");
    logical_buffer_collection->CreateBufferCollectionToken(
        logical_buffer_collection, std::numeric_limits<uint32_t>::max(),
        std::move(buffer_collection_token_request));
}

// static
//
// The buffer_collection_token is the client end of the BufferCollectionToken
// which the client is exchanging for the BufferCollection (which the client is
// passing the server end of in buffer_collection_request).
//
// However, before we convert the client's token into a BufferCollection and
// start processing the messages the client may have already sent toward the
// BufferCollection, we want to process all the messages the client may have
// already sent toward the BufferCollectionToken.  This comes up because the
// BufferCollectionToken and Allocator2 are separate channels.
//
// We know that fidl_server will process all messages before it processes the
// close - it intentionally delays noticing the close until no messages are
// available to read.
//
// So this method will close the buffer_collection_token and when it closes via
// normal FIDL processing path, the token will remember the
// buffer_collection_request to essentially convert itself into.
void LogicalBufferCollection::BindSharedCollection(
    Device* parent_device,
    zx::channel buffer_collection_token,
    zx::channel buffer_collection_request) {
    ZX_DEBUG_ASSERT(buffer_collection_token);
    ZX_DEBUG_ASSERT(buffer_collection_request);

    zx_koid_t token_client_koid;
    zx_koid_t token_server_koid;
    zx_status_t status = get_channel_koids(
        buffer_collection_token, &token_client_koid, &token_server_koid);
    if (status != ZX_OK) {
        // ~buffer_collection_token
        // ~buffer_collection_request
        return;
    }

    BufferCollectionToken* token =
        parent_device->FindTokenByServerChannelKoid(token_server_koid);
    if (!token) {
        // ~buffer_collection_token
        // ~buffer_collection_request
        return;
    }

    // This will token->FailAsync() if the token has already got one, or if the
    // token already saw token->Close().
    token->SetBufferCollectionRequest(std::move(buffer_collection_request));

    // At this point, the token will process the rest of its previously queued
    // messages (from client to server), and then will convert the token into
    // a BufferCollection (view).  That conversion happens async shortly in
    // BindSharedCollectionInternal() (unless the LogicalBufferCollection fails
    // before then, in which case everything just gets deleted).
    //
    // ~buffer_collection_token here closes the client end of the token, but we
    // still process the rest of the queued messages before we process the
    // close.
    //
    // ~buffer_collection_token
}

void LogicalBufferCollection::CreateBufferCollectionToken(
    fbl::RefPtr<LogicalBufferCollection> self, uint32_t rights_attenuation_mask,
    zx::channel buffer_collection_token_request) {
    auto token = BufferCollectionToken::Create(
        parent_device_, self, rights_attenuation_mask);
    token->SetErrorHandler([this, token_ptr = token.get()](zx_status_t status) {
        // Clean close from FIDL channel point of view is ZX_ERR_PEER_CLOSED,
        // and ZX_OK is never passed to the error handler.
        ZX_DEBUG_ASSERT(status != ZX_OK);

        // We know |this| is alive because the token is alive and the token has
        // a fbl::RefPtr<LogicalBufferCollection>.  The token is alive because
        // the token is still in token_views_.
        //
        // Any other deletion of the token_ptr out of token_views_ (outside of
        // this error handler) doesn't run this error handler.
        //
        // TODO(dustingreen): Switch to contains() when C++20.
        ZX_DEBUG_ASSERT(token_views_.find(token_ptr) != token_views_.end());

        zx::channel buffer_collection_request =
            token_ptr->TakeBufferCollectionRequest();

        if (!(status == ZX_ERR_PEER_CLOSED &&
              (token_ptr->is_done() || buffer_collection_request))) {
            // We don't have to explicitly remove token from token_views_
            // because Fail() will token_views_.clear().
            //
            // A token whose error handler sees anything other than clean close
            // with is_done() implies LogicalBufferCollection failure.  The
            // ability to detect unexpected closure of a token is a main reason
            // we use a channel for BufferCollectionToken instead of an
            // eventpair.
            Fail("Token failure causing LogicalBufferCollection failure - "
                 "status: %d",
                 status);
            return;
        }

        // At this point we know the token channel was closed cleanly, and that
        // before the client's closing the channel, the client did a
        // token::Close() or allocator::BindSharedCollection().
        ZX_DEBUG_ASSERT(status == ZX_ERR_PEER_CLOSED &&
                        (token_ptr->is_done() || buffer_collection_request));
        // BufferCollectionToken enforces that these never both become true; the
        // BufferCollectionToken will fail instead.
        ZX_DEBUG_ASSERT(!(token_ptr->is_done() && buffer_collection_request));

        if (!buffer_collection_request) {
            // This was a token::Close().  In this case we want to stop tracking
            // the token now that we've processed all its previously-queued
            // inbound messages.  This might be the last token, so we
            // MaybeAllocate().  This path isn't a failure.
            auto self = token_ptr->parent_shared();
            ZX_DEBUG_ASSERT(self.get() == this);
            token_views_.erase(token_ptr);
            MaybeAllocate();
            // ~self - might delete this
        }

        // At this point we know that this was a BindSharedCollection().  We
        // need to convert the BufferCollectionToken into a BufferCollection.
        //
        // ~token_ptr during this call
        BindSharedCollectionInternal(token_ptr,
                                     std::move(buffer_collection_request));
    });
    auto token_ptr = token.get();
    token_views_.insert({token_ptr, std::move(token)});

    zx_koid_t server_koid;
    zx_koid_t client_koid;
    zx_status_t status = get_channel_koids(buffer_collection_token_request,
                                           &server_koid, &client_koid);
    if (status != ZX_OK) {
        Fail("get_channel_koids() failed - status: %d", status);
        return;
    }
    token_ptr->SetServerKoid(server_koid);

    LogInfo("CreateBufferCollectionToken() - server_koid: %lu",
            token_ptr->server_koid());
    token_ptr->Bind(std::move(buffer_collection_token_request));
}

void LogicalBufferCollection::OnSetConstraints() {
    MaybeAllocate();
    return;
}

LogicalBufferCollection::AllocationResult
LogicalBufferCollection::allocation_result() {
    ZX_DEBUG_ASSERT(
        has_allocation_result_ ||
        (allocation_result_status_ == ZX_OK && !allocation_result_info_));
    return {
        .buffer_collection_info = allocation_result_info_.get(),
        .status = allocation_result_status_,
    };
}

LogicalBufferCollection::LogicalBufferCollection(Device* parent_device)
    : parent_device_(parent_device), constraints_(Constraints::Null) {
    // nothing else to do here
}

LogicalBufferCollection::~LogicalBufferCollection() {
    LogInfo("~LogicalBufferCollection");
    // Every entry in these collections keeps a
    // fbl::RefPtr<LogicalBufferCollection>, so these should both already be
    // empty.
    ZX_DEBUG_ASSERT(token_views_.empty());
    ZX_DEBUG_ASSERT(collection_views_.empty());
}

void LogicalBufferCollection::Fail(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vLog(true, "LogicalBufferCollection", "fail", format, args);
    va_end(args);

    // Close all the associated channels.  We do this by swapping into local
    // collections and clearing those, since deleting the items in the
    // collections will delete |this|.
    TokenMap local_token_views;
    token_views_.swap(local_token_views);
    CollectionMap local_collection_views;
    collection_views_.swap(local_collection_views);

    // |this| is very likely to be deleted during these calls to clear().  The
    // only exception is if the caller of Fail() happens to have its own
    // temporary fbl::RefPtr<LogicalBufferCollection> on the stack.
    local_token_views.clear();
    local_collection_views.clear();
}

void LogicalBufferCollection::LogInfo(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vLog(false, "LogicalBufferCollection", "info", format, args);
    va_end(args);
}

void LogicalBufferCollection::LogError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vLog(true, "LogicalBufferCollection", "error", format, args);
    va_end(args);
}

void LogicalBufferCollection::MaybeAllocate() {
    if (is_allocate_attempted_) {
        // Allocate was already attempted.
        return;
    }
    if (!token_views_.empty()) {
        // All tokens must be converted into BufferCollection views or Close()ed
        // before allocation will happen.
        return;
    }
    if (collection_views_.empty()) {
        // No point in allocating if there aren't any BufferCollection views
        // left either.
        return;
    }
    // Sweep looking for any views that haven't set constraints.
    for (auto& [key, value] : collection_views_) {
        if (!key->set_constraints_seen()) {
            return;
        }
    }
    // All the views have seen SetConstraints(), and there are no tokens left.
    // Regardless of whether allocation succeeds or fails, we remember we've
    // started an attempt to allocate so we don't attempt again.
    is_allocate_attempted_ = true;
    TryAllocate();
    return;
}

// This only runs on a clean stack.
void LogicalBufferCollection::TryAllocate() {
    // If we're here it means we still have collection_views_, because if the
    // last collection view disappeared we would have run ~this which would have
    // cleared the Post() canary so this method woudn't be running.
    ZX_DEBUG_ASSERT(!collection_views_.empty());

    if (!CombineConstraints()) {
        // It's impossible to combine the constraints due to incompatible
        // constraints, or all participants set null constraints.
        SetFailedAllocationResult(ZX_ERR_NOT_SUPPORTED);
        return;
    }
    ZX_DEBUG_ASSERT(!!constraints_);

    zx_status_t allocate_result = ZX_OK;
    BufferCollectionInfo allocation = Allocate(&allocate_result);
    if (!allocation) {
        ZX_DEBUG_ASSERT(allocate_result != ZX_OK);
        SetFailedAllocationResult(allocate_result);
        return;
    }
    ZX_DEBUG_ASSERT(allocate_result == ZX_OK);

    SetAllocationResult(std::move(allocation));
    return;
}

void LogicalBufferCollection::SetFailedAllocationResult(zx_status_t status) {
    ZX_DEBUG_ASSERT(status != ZX_OK);

    // Only set result once.
    ZX_DEBUG_ASSERT(!has_allocation_result_);
    // allocation_result_status_ is initialized to ZX_OK, so should still be set
    // that way.
    ZX_DEBUG_ASSERT(allocation_result_status_ == ZX_OK);

    allocation_result_status_ = status;
    // Was initialized to nullptr.
    ZX_DEBUG_ASSERT(!allocation_result_info_);
    has_allocation_result_ = true;
    SendAllocationResult();
    return;
}

void LogicalBufferCollection::SetAllocationResult(BufferCollectionInfo info) {
    // Setting null constraints as the success case isn't allowed.  That's
    // considered a failure.  At least one participant must specify non-null
    // constraints.
    ZX_DEBUG_ASSERT(info);

    // Only set result once.
    ZX_DEBUG_ASSERT(!has_allocation_result_);
    // allocation_result_status_ is initialized to ZX_OK, so should still be set
    // that way.
    ZX_DEBUG_ASSERT(allocation_result_status_ == ZX_OK);

    allocation_result_status_ = ZX_OK;
    allocation_result_info_ = std::move(info);
    has_allocation_result_ = true;
    SendAllocationResult();
    return;
}

void LogicalBufferCollection::SendAllocationResult() {
    ZX_DEBUG_ASSERT(has_allocation_result_);
    ZX_DEBUG_ASSERT(token_views_.empty());
    ZX_DEBUG_ASSERT(!collection_views_.empty());

    for (auto& [key, value] : collection_views_) {
        // May as well assert since we can.
        ZX_DEBUG_ASSERT(key->set_constraints_seen());
        key->OnBuffersAllocated();
    }

    if (allocation_result_status_ != ZX_OK) {
        Fail("LogicalBufferCollection::SendAllocationResult() done sending "
             "allocation failure - now auto-failing self.");
        return;
    }
}

void LogicalBufferCollection::BindSharedCollectionInternal(
    BufferCollectionToken* token, zx::channel buffer_collection_request) {
    auto self = token->parent_shared();
    ZX_DEBUG_ASSERT(self.get() == this);
    auto collection = BufferCollection::Create(self);
    collection->SetErrorHandler(
        [this, collection_ptr = collection.get()](zx_status_t status) {
            // status passed to an error handler is never ZX_OK.  Clean close is
            // ZX_ERR_PEER_CLOSED.
            ZX_DEBUG_ASSERT(status != ZX_OK);

            // We know collection_ptr is still alive because collection_ptr is
            // still in collection_views_.  We know this is still alive because
            // this has a RefPtr<> ref from collection_ptr.
            //
            // TODO(dustingreen): Switch to contains() when C++20.
            ZX_DEBUG_ASSERT(collection_views_.find(collection_ptr) !=
                            collection_views_.end());

            // The BufferCollection may have had Close() called on it, in which
            // case closure of the BufferCollection doesn't cause
            // LogicalBufferCollection failure.  Or, Close() wasn't called and
            // the LogicalBufferCollection is out of here.

            if (!(status == ZX_ERR_PEER_CLOSED && collection_ptr->is_done())) {
                // We don't have to explicitly remove collection from
                // collection_views_ because Fail() will
                // collection_views_.clear().
                //
                // A BufferCollection view whose error handler runs implies
                // LogicalBufferCollection failure.
                Fail("BufferCollection (view) failure (or closure without "
                     "Close()) causing "
                     "LogicalBufferCollection failure - status: %d",
                     status);
                return;
            }

            // At this point we know the collection_ptr is cleanly done (Close()
            // was sent from client) and can be removed from the set of tracked
            // collections.
            auto self = collection_ptr->parent_shared();
            ZX_DEBUG_ASSERT(self.get() == this);
            collection_views_.erase(collection_ptr);
            MaybeAllocate();
            return;
        });
    auto collection_ptr = collection.get();
    collection_views_.insert({collection_ptr, std::move(collection)});
    // ~BufferCollectionToken calls UntrackTokenKoid().
    token_views_.erase(token);
    collection_ptr->Bind(std::move(buffer_collection_request));
}

bool LogicalBufferCollection::CombineConstraints() {
    // This doesn't necessarily mean that any of the collection_views_ have
    // non-null constraints though.
    ZX_DEBUG_ASSERT(!collection_views_.empty());

    auto iter =
        std::find_if(collection_views_.begin(), collection_views_.end(),
                     [](auto& pair) { return !!pair.first->constraints(); });
    if (iter == collection_views_.end()) {
        // This is a failure.  At least one participant must provide
        // constraints.
        return false;
    }

    if (!CheckBufferCollectionConstraints(iter->first->constraints())) {
        return false;
    }

    Constraints result =
        BufferCollectionConstraintsClone(iter->first->constraints());
    ++iter;

    for (; iter != collection_views_.end(); ++iter) {
        if (!iter->first->constraints()) {
            continue;
        }
        if (!CheckBufferCollectionConstraints(iter->first->constraints())) {
            return false;
        }
        if (!AccumulateConstraintBufferCollection(result.get(),
                                                  iter->first->constraints())) {
            // This is a failure.  The space of permitted settings contains no
            // points.
            return false;
        }
    }

    SanitizeBufferCollectionConstraints(result.get());

    constraints_ = std::move(result);
    return true;
}

bool LogicalBufferCollection::CheckBufferCollectionConstraints(
    const fuchsia_sysmem_BufferCollectionConstraints* constraints) {
    // At least one usage bit must be specified by any participant that
    // specifies constraints.
    if (constraints->usage.cpu == 0 && constraints->usage.vulkan == 0 &&
        constraints->usage.display == 0 && constraints->usage.video == 0) {
        LogError("At least one usage bit must be set.");
        return false;
    }
    for (uint32_t i = 0; i < constraints->image_format_constraints_count; ++i) {
        if (!CheckImageFormatConstraints(
                &constraints->image_format_constraints[i])) {
            return false;
        }
    }
    return true;
}

bool LogicalBufferCollection::CheckImageFormatConstraints(
    const fuchsia_sysmem_ImageFormatConstraints* constraints) {
    if (constraints->pixel_format.type ==
        fuchsia_sysmem_PixelFormatType_INVALID) {
        LogError("PixelFormatType INVALID not allowed");
        return false;
    }
    if (constraints->pixel_format.has_format_modifier) {
        LogError("has_format_modifier == true not yet implemented");
        return false;
    }
    if (!constraints->color_spaces_count) {
        LogError("color_spaces_count == 0 not allowed");
        return false;
    }
    if (constraints->layers != 1) {
        LogError("layers != 1 is not yet implemented");
        return false;
    }

    if (!IsNonZeroPowerOf2(constraints->coded_width_divisor)) {
        LogError("non-power-of-2 coded_width_divisor not supported");
        return false;
    }
    if (!IsNonZeroPowerOf2(constraints->coded_height_divisor)) {
        LogError("non-power-of-2 coded_width_divisor not supported");
        return false;
    }
    if (!IsNonZeroPowerOf2(constraints->bytes_per_row_divisor)) {
        LogError("non-power-of-2 bytes_per_row_divisor not supported");
        return false;
    }
    if (!IsNonZeroPowerOf2(constraints->display_width_divisor)) {
        LogError("non-power-of-2 display_width_divisor not supported");
        return false;
    }
    if (!IsNonZeroPowerOf2(constraints->display_height_divisor)) {
        LogError("non-power-of-2 display_height_divisor not supported");
        return false;
    }

    for (uint32_t i = 0; i < constraints->color_spaces_count; ++i) {
        if (!ImageFormatIsSupportedColorSpaceForPixelFormat(
                constraints->color_space[i], constraints->pixel_format)) {
            LogError("!ImageFormatIsSupportedColorSpaceForPixelFormat() "
                     "color_space.type: %u "
                     "pixel_format.type: %u",
                     constraints->color_space[i].type,
                     constraints->pixel_format.type);
            return false;
        }
    }
    // TODO(dustingreen): Check compatibility of color_space[] entries vs. the
    // pixel_format.  In particular, 2020 and 2100 don't have 8 bpp, only 10 or
    // 12 bpp, while a given PixelFormat.type is a specific bpp.  There's
    // probably no reason to allow 2020 or 2100 to be specified along with a
    // PixelFormat.type that's 8 bpp for example.

    return true;
}

void LogicalBufferCollection::SanitizeBufferCollectionConstraints(
    fuchsia_sysmem_BufferCollectionConstraints* constraints) {
    for (uint32_t i = 0; i < constraints->image_format_constraints_count; ++i) {
        // Ensure min_bytes_per_row is sufficient to fit min_coded_width.
        fuchsia_sysmem_ImageFormatConstraints* image_format_constraints =
            &constraints->image_format_constraints[i];
        uint32_t min_bytes_per_row_given_min_width =
            ImageFormatStrideBytesPerWidthPixel(
                &image_format_constraints->pixel_format) *
            image_format_constraints->min_coded_width;
        image_format_constraints->min_bytes_per_row =
            std::max(image_format_constraints->min_bytes_per_row,
                     min_bytes_per_row_given_min_width);
    }
}

LogicalBufferCollection::Constraints
LogicalBufferCollection::BufferCollectionConstraintsClone(
    const fuchsia_sysmem_BufferCollectionConstraints* input) {
    // There are no handles in BufferCollectionConstraints, so just copy the
    // payload.  If any handles are added later we'll have to fix this up.
    return Constraints(*input);
}

LogicalBufferCollection::ImageFormatConstraints
LogicalBufferCollection::ImageFormatConstraintsClone(
    const fuchsia_sysmem_ImageFormatConstraints* input) {
    // There are no handles in ImageFormatConstraints, so just copy the
    // payload.  If any handles are added later we'll have to fix this up.
    return ImageFormatConstraints(*input);
}

// |acc| accumulated constraints so far
//
// |c| additional constraint to aggregate into acc
bool LogicalBufferCollection::AccumulateConstraintBufferCollection(
    fuchsia_sysmem_BufferCollectionConstraints* acc,
    const fuchsia_sysmem_BufferCollectionConstraints* c) {
    acc->usage.cpu |= c->usage.cpu;
    acc->usage.vulkan |= c->usage.vulkan;
    acc->usage.display |= c->usage.display;
    acc->usage.video |= c->usage.video;

    acc->min_buffer_count_for_camping += c->min_buffer_count_for_camping;
    acc->min_buffer_count_for_dedicated_slack +=
        c->min_buffer_count_for_dedicated_slack;
    acc->min_buffer_count_for_shared_slack =
        std::max(acc->min_buffer_count_for_shared_slack,
                 c->min_buffer_count_for_shared_slack);

    if (!acc->has_buffer_memory_constraints) {
        if (c->has_buffer_memory_constraints) {
            // struct copy
            acc->buffer_memory_constraints = c->buffer_memory_constraints;
            acc->has_buffer_memory_constraints = true;
        }
    } else {
        ZX_DEBUG_ASSERT(acc->has_buffer_memory_constraints);
        if (c->has_buffer_memory_constraints) {
            if (!AccumulateConstraintBufferMemory(
                    &acc->buffer_memory_constraints,
                    &c->buffer_memory_constraints)) {
                return false;
            }
        }
    }

    // Reject secure_required in combination with any CPU usage, since CPU usage
    // isn't possible given secure memory.
    if (acc->has_buffer_memory_constraints &&
        acc->buffer_memory_constraints.secure_required &&
        IsCpuUsage(acc->usage)) {
        return false;
    }

    if (!acc->image_format_constraints_count) {
        for (uint32_t i = 0; i < c->image_format_constraints_count; ++i) {
            // struct copy
            acc->image_format_constraints[i] = c->image_format_constraints[i];
        }
        acc->image_format_constraints_count = c->image_format_constraints_count;
    } else {
        ZX_DEBUG_ASSERT(acc->image_format_constraints_count);
        if (c->image_format_constraints_count) {
            if (!AccumulateConstraintImageFormats(
                    &acc->image_format_constraints_count,
                    acc->image_format_constraints,
                    c->image_format_constraints_count,
                    c->image_format_constraints)) {
                // We return false if we've seen non-zero
                // image_format_constraint_count from at least one participant
                // but among non-zero image_format_constraint_count participants
                // since then the overlap has dropped to empty set.
                //
                // This path is taken when there are completely non-overlapping
                // PixelFormats and also when PixelFormat(s) overlap but none
                // of those have any non-empty settings space remaining.  In
                // that case we've removed the PixelFormat from consideration
                // despite it being common among participants (so far).
                return false;
            }
            ZX_DEBUG_ASSERT(acc->image_format_constraints_count);
        }
    }

    // acc->image_format_constraints_count == 0 is allowed here, when all
    // participants had image_format_constraints_count == 0.
    return true;
}

bool LogicalBufferCollection::AccumulateConstraintBufferMemory(
    fuchsia_sysmem_BufferMemoryConstraints* acc,
    const fuchsia_sysmem_BufferMemoryConstraints* c) {
    acc->min_size_bytes = std::max(acc->min_size_bytes, c->min_size_bytes);
    acc->max_size_bytes = std::min(acc->max_size_bytes, c->max_size_bytes);
    if (acc->min_size_bytes > acc->max_size_bytes) {
        LogError("min_size_bytes > max_size_bytes");
        return false;
    }

    acc->physically_contiguous_required = acc->physically_contiguous_required ||
                                          c->physically_contiguous_required;

    acc->secure_required = acc->secure_required || c->secure_required;
    acc->secure_permitted = acc->secure_permitted && c->secure_permitted;
    if (acc->secure_required && !acc->secure_permitted) {
        LogError("secure_required && !secure_permitted");
        return false;
    }
    return true;
}

bool LogicalBufferCollection::AccumulateConstraintImageFormats(
    uint32_t* acc_count, fuchsia_sysmem_ImageFormatConstraints acc[],
    uint32_t c_count, const fuchsia_sysmem_ImageFormatConstraints c[]) {
    // Remove any pixel_format in acc that's not in c.  Process any format
    // that's in both.  If processing the format results in empty set for that
    // format, pretend as if the format wasn't in c and remove that format from
    // acc.  If acc ends up with zero formats, return false.

    // This method doesn't get called unless there's at least one format in
    // acc.
    ZX_DEBUG_ASSERT(*acc_count);

    for (uint32_t ai = 0; ai < *acc_count; ++ai) {
        uint32_t ci;
        for (ci = 0; ci < c_count; ++ci) {
            if (ImageFormatIsPixelFormatEqual(acc[ai].pixel_format,
                                              c[ci].pixel_format)) {
                if (!AccumulateConstraintImageFormat(&acc[ai], &c[ci])) {
                    // Pretend like the format wasn't in c to begin with, so
                    // this format gets removed from acc.  Only if this results
                    // in zero formats in acc do we end up returning false.
                    ci = c_count;
                    break;
                }
                // We found the format in c and processed the format without
                // that resulting in empty set; break so we can move on to the
                // next format.
                break;
            }
        }
        if (ci == c_count) {
            // remove from acc because not found in c
            --(*acc_count);
            // struct copy of formerly last item on top of the item being
            // removed
            acc[ai] = acc[*acc_count];
            // adjust ai to force current index to be processed again as it's
            // now a different item
            --ai;
        }
    }

    if (!*acc_count) {
        LogError("all pixel_format(s) eliminated");
        return false;
    }

    return true;
}

bool LogicalBufferCollection::AccumulateConstraintImageFormat(
    fuchsia_sysmem_ImageFormatConstraints* acc,
    const fuchsia_sysmem_ImageFormatConstraints* c) {
    ZX_DEBUG_ASSERT(
        ImageFormatIsPixelFormatEqual(acc->pixel_format, c->pixel_format));
    // Checked previously.
    ZX_DEBUG_ASSERT(acc->color_spaces_count);
    // Checked previously.
    ZX_DEBUG_ASSERT(c->color_spaces_count);

    if (!AccumulateConstraintColorSpaces(
            &acc->color_spaces_count, acc->color_space, c->color_spaces_count,
            c->color_space)) {
        return false;
    }
    // Else AccumulateConstraintColorSpaces() would have returned false.
    ZX_DEBUG_ASSERT(acc->color_spaces_count);

    acc->min_coded_width = std::max(acc->min_coded_width, c->min_coded_width);
    acc->max_coded_width = std::min(acc->max_coded_width, c->max_coded_width);
    if (acc->min_coded_width > acc->max_coded_width) {
        LogError("min_coded_width > max_coded_width");
        return false;
    }

    acc->min_coded_height =
        std::max(acc->min_coded_height, c->min_coded_height);
    acc->max_coded_height =
        std::min(acc->max_coded_height, c->max_coded_height);
    if (acc->min_coded_height > acc->max_coded_height) {
        LogError("min_coded_height > max_coded_height");
        return false;
    }

    acc->min_bytes_per_row =
        std::max(acc->min_bytes_per_row, c->min_bytes_per_row);
    acc->max_bytes_per_row =
        std::min(acc->max_bytes_per_row, c->max_bytes_per_row);
    if (acc->min_bytes_per_row > acc->max_bytes_per_row) {
        LogError("min_bytes_per_row > max_bytes_per_row");
        return false;
    }

    acc->max_coded_width_times_coded_height =
        std::min(acc->max_coded_width_times_coded_height,
                 c->max_coded_width_times_coded_height);
    if (acc->min_coded_width * acc->min_coded_height >
        acc->max_coded_width_times_coded_height) {
        LogError("min_coded_width * min_coded_height > "
                 "max_coded_width_times_coded_height");
        return false;
    }

    // Checked previously.
    ZX_DEBUG_ASSERT(acc->layers == 1);
    if (acc->layers != 1) {
        LogError("layers != 1 is not yet implemented");
        return false;
    }

    acc->coded_width_divisor =
        std::max(acc->coded_width_divisor, c->coded_width_divisor);
    acc->coded_width_divisor =
        std::max(acc->coded_width_divisor,
                 ImageFormatCodedWidthMinDivisor(&acc->pixel_format));

    acc->coded_height_divisor =
        std::max(acc->coded_height_divisor, c->coded_height_divisor);
    acc->coded_height_divisor =
        std::max(acc->coded_height_divisor,
                 ImageFormatCodedHeightMinDivisor(&acc->pixel_format));

    acc->bytes_per_row_divisor =
        std::max(acc->bytes_per_row_divisor, c->bytes_per_row_divisor);
    acc->bytes_per_row_divisor =
        std::max(acc->bytes_per_row_divisor,
                 ImageFormatSampleAlignment(&acc->pixel_format));

    acc->start_offset_divisor =
        std::max(acc->start_offset_divisor, c->start_offset_divisor);
    acc->start_offset_divisor =
        std::max(acc->start_offset_divisor,
                 ImageFormatSampleAlignment(&acc->pixel_format));
    if (acc->start_offset_divisor > PAGE_SIZE) {
        LogError(
            "support for start_offset_divisor > PAGE_SIZE not yet implemented");
        return false;
    }

    acc->display_width_divisor =
        std::max(acc->display_width_divisor, c->display_width_divisor);
    acc->display_height_divisor =
        std::max(acc->display_height_divisor, c->display_height_divisor);

    return true;
}

bool LogicalBufferCollection::AccumulateConstraintColorSpaces(
    uint32_t* acc_count, fuchsia_sysmem_ColorSpace acc[], uint32_t c_count,
    const fuchsia_sysmem_ColorSpace c[]) {
    // Remove any color_space in acc that's not in c.  If zero color spaces
    // remain in acc, return false.

    for (uint32_t ai = 0; ai < *acc_count; ++ai) {
        uint32_t ci;
        for (ci = 0; ci < c_count; ++ci) {
            if (IsColorSpaceEqual(acc[ai], c[ci])) {
                // We found the color space in c.  Break so we can move on to
                // the next color space.
                break;
            }
        }
        if (ci == c_count) {
            // remove from acc because not found in c
            --(*acc_count);
            // struct copy of formerly last item on top of the item being
            // removed
            acc[ai] = acc[*acc_count];
            // adjust ai to force current index to be processed again as it's
            // now a different item
            --ai;
        }
    }

    if (!*acc_count) {
        LogError("Zero color_space overlap");
        return false;
    }

    return true;
}

bool LogicalBufferCollection::IsColorSpaceEqual(
    const fuchsia_sysmem_ColorSpace& a, const fuchsia_sysmem_ColorSpace& b) {
    return a.type == b.type;
}

BufferCollection::BufferCollectionInfo
LogicalBufferCollection::Allocate(zx_status_t* allocation_result) {
    ZX_DEBUG_ASSERT(constraints_);
    ZX_DEBUG_ASSERT(allocation_result);

    // Unless fails later.
    *allocation_result = ZX_OK;

    BufferCollection::BufferCollectionInfo result(
        BufferCollection::BufferCollectionInfo::Default);

    result->buffer_count = constraints_->min_buffer_count_for_camping +
                           constraints_->min_buffer_count_for_dedicated_slack +
                           constraints_->min_buffer_count_for_shared_slack;
    uint64_t min_size_bytes = 0;
    uint64_t max_size_bytes = std::numeric_limits<uint64_t>::max();

    fuchsia_sysmem_SingleBufferSettings* settings = &result->settings;
    fuchsia_sysmem_BufferMemorySettings* buffer_settings =
        &settings->buffer_settings;

    // It's allowed for zero participants to have buffer_memory_constraints, as
    // long as at least one participant has image_format_constraint_count != 0.
    if (!constraints_->has_buffer_memory_constraints &&
        !constraints_->image_format_constraints_count) {
        // Too unconstrained...  We refuse to allocate buffers without any size
        // bounds from any participant.  At least one particpant must provide
        // some form of size bounds (in terms of buffer size bounds or in terms
        // of image size bounds).
        LogError("at least one participant must specify "
                 "buffer_memory_constraints or "
                 "image_format_constraints");
        *allocation_result = ZX_ERR_NOT_SUPPORTED;
        return BufferCollection::BufferCollectionInfo(
            BufferCollection::BufferCollectionInfo::Null);
    }
    if (constraints_->has_buffer_memory_constraints) {
        const fuchsia_sysmem_BufferMemoryConstraints* buffer_constraints =
            &constraints_->buffer_memory_constraints;
        buffer_settings->is_physically_contiguous =
            buffer_constraints->physically_contiguous_required;
        // checked previously
        ZX_DEBUG_ASSERT(!(buffer_constraints->secure_required &&
                          !buffer_constraints->secure_permitted));
        // checked previously
        ZX_DEBUG_ASSERT(!(buffer_constraints->secure_required &&
                          IsCpuUsage(constraints_->usage)));
        buffer_settings->is_secure = buffer_constraints->secure_required;
        // We can't fill out buffer_settings yet because that also depends on
        // ImageFormatConstraints.  We do need the min and max from here though.
        min_size_bytes = buffer_constraints->min_size_bytes;
        max_size_bytes = buffer_constraints->max_size_bytes;
    }

    // It's allowed for zero participants to have any ImageFormatConstraint(s),
    // in which case the combined constraints_ will have zero (and that's fine,
    // when allocating raw buffers that don't need any ImageFormatConstraint).
    //
    // At least for now, we pick which PixelFormat to use before determining if
    // the constraints associated with that PixelFormat imply a buffer size
    // range in min_size_bytes..max_size_bytes.
    if (constraints_->image_format_constraints_count) {
        // Pick the best ImageFormatConstraints.
        uint32_t best_index = 0;
        for (uint32_t i = 1; i < constraints_->image_format_constraints_count;
             ++i) {
            if (CompareImageFormatConstraintsByIndex(i, best_index) > 0) {
                best_index = i;
            }
        }
        // struct copy - if right hand side's clone results in any duplicated
        // handles, those will be owned by result.
        settings->image_format_constraints =
            *ImageFormatConstraintsClone(
                 &constraints_->image_format_constraints[best_index])
                 .get();
        settings->has_image_format_constraints = true;
    }

    // Compute the min buffer size implied by image_format_constraints, so we
    // ensure the buffers can hold the min-size image.
    if (settings->has_image_format_constraints) {
        const fuchsia_sysmem_ImageFormatConstraints* constraints =
            &settings->image_format_constraints;
        fuchsia_sysmem_ImageFormat_2 min_image{};

        // struct copy
        min_image.pixel_format = constraints->pixel_format;

        min_image.coded_width =
            AlignUp(constraints->min_coded_width,
                    FieldDefault1(constraints->coded_width_divisor));
        if (min_image.coded_width > constraints->max_coded_width) {
            LogError(
                "coded_width_divisor caused coded_width > max_coded_width");
            *allocation_result = ZX_ERR_NOT_SUPPORTED;
            return BufferCollection::BufferCollectionInfo(
                BufferCollection::BufferCollectionInfo::Null);
        }
        min_image.coded_height =
            AlignUp(constraints->min_coded_height,
                    FieldDefault1(constraints->coded_height_divisor));
        if (min_image.coded_height > constraints->max_coded_height) {
            LogError(
                "coded_height_divisor caused coded_height > max_coded_height");
            *allocation_result = ZX_ERR_NOT_SUPPORTED;
            return BufferCollection::BufferCollectionInfo(
                BufferCollection::BufferCollectionInfo::Null);
        }
        min_image.bytes_per_row =
            AlignUp(constraints->min_bytes_per_row,
                    FieldDefault1(constraints->bytes_per_row_divisor));
        if (min_image.bytes_per_row > constraints->max_bytes_per_row) {
            LogError("bytes_per_row_divisor caused bytes_per_row > "
                     "max_bytes_per_row");
            *allocation_result = ZX_ERR_NOT_SUPPORTED;
            return BufferCollection::BufferCollectionInfo(
                BufferCollection::BufferCollectionInfo::Null);
        }

        if (min_image.coded_width * min_image.coded_height >
            constraints->max_coded_width_times_coded_height) {
            LogError("coded_width * coded_height > "
                     "max_coded_width_times_coded_height");
            *allocation_result = ZX_ERR_NOT_SUPPORTED;
            return BufferCollection::BufferCollectionInfo(
                BufferCollection::BufferCollectionInfo::Null);
        }

        // These don't matter for computing size in bytes.
        ZX_DEBUG_ASSERT(min_image.display_width == 0);
        ZX_DEBUG_ASSERT(min_image.display_height == 0);

        // This is the only supported value for layers for now.
        min_image.layers = 1;

        // Checked previously.
        ZX_DEBUG_ASSERT(constraints->color_spaces_count >= 1);
        // This doesn't matter for computing size in bytes, as we trust the
        // pixel_format to fully specify the image size.  But set it to the
        // first ColorSpace anyway, just so the color_space.type is a valid
        // value.
        //
        // struct copy
        min_image.color_space = constraints->color_space[0];

        uint64_t image_min_size_bytes = ImageFormatImageSize(&min_image);

        if (image_min_size_bytes > min_size_bytes) {
            if (image_min_size_bytes > max_size_bytes) {
                LogError("image_min_size_bytes > max_size_bytes");
                *allocation_result = ZX_ERR_NOT_SUPPORTED;
                return BufferCollection::BufferCollectionInfo(
                    BufferCollection::BufferCollectionInfo::Null);
            }
            min_size_bytes = image_min_size_bytes;
            ZX_DEBUG_ASSERT(min_size_bytes <= max_size_bytes);
        }
    }

    if (min_size_bytes == 0) {
        LogError("min_size_bytes == 0");
        *allocation_result = ZX_ERR_NOT_SUPPORTED;
        return BufferCollection::BufferCollectionInfo(
            BufferCollection::BufferCollectionInfo::Null);
    }

    // For purposes of enforcing max_size_bytes, we intentionally don't care
    // that a VMO can only be a multiple of page size.

    uint64_t total_size_bytes = min_size_bytes * result->buffer_count;
    if (total_size_bytes > kMaxTotalSizeBytesPerCollection) {
        LogError("total_size_bytes > kMaxTotalSizeBytesPerCollection");
        *allocation_result = ZX_ERR_NO_MEMORY;
        return BufferCollection::BufferCollectionInfo(
            BufferCollection::BufferCollectionInfo::Null);
    }

    if (min_size_bytes > kMaxSizeBytesPerBuffer) {
        LogError("min_size_bytes > kMaxSizeBytesPerBuffer");
        *allocation_result = ZX_ERR_NO_MEMORY;
        return BufferCollection::BufferCollectionInfo(
            BufferCollection::BufferCollectionInfo::Null);
    }
    ZX_DEBUG_ASSERT(min_size_bytes <= std::numeric_limits<uint32_t>::max());

    // Now that min_size_bytes accounts for any ImageFormatConstraints, we can
    // just allocate min_size_bytes buffers.
    buffer_settings->size_bytes = static_cast<uint32_t>(min_size_bytes);

    for (uint32_t i = 0; i < result->buffer_count; ++i) {
        // Assign directly into result to benefit from FidlStruct<> management
        // of handle lifetime.
        zx::vmo vmo;
        zx_status_t allocate_result = AllocateVmo(settings, &vmo);
        if (allocate_result != ZX_OK) {
            ZX_DEBUG_ASSERT(allocate_result == ZX_ERR_NO_MEMORY);
            LogError("AllocateVmo() failed - status: %d", allocate_result);
            // In release sanitize error code to ZX_ERR_NO_MEMORY regardless of
            // what AllocateVmo() returned.
            *allocation_result = ZX_ERR_NO_MEMORY;
            return BufferCollection::BufferCollectionInfo(
                BufferCollection::BufferCollectionInfo::Null);
        }
        // Transfer ownership from zx::vmo to FidlStruct<>.
        result->buffers[i].vmo = vmo.release();
    }

    ZX_DEBUG_ASSERT(*allocation_result == ZX_OK);
    return result;
}

zx_status_t LogicalBufferCollection::AllocateVmo(
    const fuchsia_sysmem_SingleBufferSettings* settings, zx::vmo* vmo) {
    if (settings->buffer_settings.is_secure) {
        ProtectedMemoryAllocator* allocator = parent_device_->protected_allocator();
        if (!allocator) {
            LogError("No protected memory allocator");
            return ZX_ERR_NOT_SUPPORTED;
        }

        zx::vmo raw_vmo;
        zx_status_t status = allocator->Allocate(settings->buffer_settings.size_bytes, &raw_vmo);
        if (status != ZX_OK) {
            LogError("Protected allocate failed - size_bytes: %u "
                     "status: %d",
                     settings->buffer_settings.size_bytes, status);
            // sanitize to ZX_ERR_NO_MEMORY regardless of why.
            status = ZX_ERR_NO_MEMORY;
            return status;
        }
        status = raw_vmo.duplicate(kSysmemVmoRights, vmo);
        if (status != ZX_OK) {
            LogError("zx::object::duplicate() failed - status: %d", status);
            return status;
        }
    } else if (settings->buffer_settings.is_physically_contiguous) {
        // TODO(dustingreen): Optionally (per board) pre-allocate a
        // physically-contiguous VMO with board-specific size, have a page heap,
        // dole out portions of that VMO or non-copy-on-write child VMOs of that
        // VMO.  For now, we just attempt to allocate contiguous when buffers
        // are allocated, which is unlikely to work after the system has been
        // running for a while and physical memory is more fragmented than early
        // during boot.
        zx::vmo raw_vmo;
        zx_status_t status = zx::vmo::create_contiguous(
            parent_device_->bti(), settings->buffer_settings.size_bytes, 0,
            &raw_vmo);
        if (status != ZX_OK) {
            LogError("zx::vmo::create_contiguous() failed - size_bytes: %u "
                     "status: %d",
                     settings->buffer_settings.size_bytes, status);
            // sanitize to ZX_ERR_NO_MEMORY regardless of why.
            status = ZX_ERR_NO_MEMORY;
            return status;
        }
        status = raw_vmo.duplicate(kSysmemVmoRights, vmo);
        if (status != ZX_OK) {
            LogError("zx::object::duplicate() failed - status: %d", status);
            return status;
        }
        // ~raw_vmo - *vmo is a duplicate with slightly-reduced rights.
    } else {
        zx::vmo raw_vmo;
        zx_status_t status =
            zx::vmo::create(settings->buffer_settings.size_bytes, 0, &raw_vmo);
        if (status != ZX_OK) {
            LogError("zx::vmo::create() failed - size_bytes: %u status: %d",
                     settings->buffer_settings.size_bytes, status);
            // sanitize to ZX_ERR_NO_MEMORY regardless of why.
            status = ZX_ERR_NO_MEMORY;
            return status;
        }
        status = raw_vmo.duplicate(kSysmemVmoRights, vmo);
        if (status != ZX_OK) {
            LogError("zx::object::duplicate() failed - status: %d", status);
            return status;
        }
        // ~raw_vmo - *vmo is a duplicate with slightly-reduced rights.
    }
    return ZX_OK;
}

// 1 means a > b, 0 means ==, -1 means a < b.
//
// TODO(dustingreen): Pay attention to constraints_->usage, by checking any
// overrides that prefer particular PixelFormat based on a usage / usage
// combination.
int32_t LogicalBufferCollection::CompareImageFormatConstraintsTieBreaker(
    const fuchsia_sysmem_ImageFormatConstraints* a,
    const fuchsia_sysmem_ImageFormatConstraints* b) {
    // If there's not any cost difference, fall back to choosing the
    // pixel_format that has the larger type enum value as a tie-breaker.

    // For now, require that there's no format modifier.  Later we'll want to
    // tie break based on that information also.
    ZX_DEBUG_ASSERT(!a->pixel_format.has_format_modifier);
    ZX_DEBUG_ASSERT(!b->pixel_format.has_format_modifier);

    int32_t raw_result = static_cast<int32_t>(a->pixel_format.type) -
                         static_cast<int32_t>(b->pixel_format.type);

    int32_t cooked_result = raw_result;
    if (cooked_result > 0) {
        cooked_result = 1;
    } else if (cooked_result < 0) {
        cooked_result = -1;
    }

    ZX_DEBUG_ASSERT(cooked_result == 0 || cooked_result == 1 ||
                    cooked_result == -1);

    return cooked_result;
}

int32_t LogicalBufferCollection::CompareImageFormatConstraintsByIndex(
    uint32_t index_a, uint32_t index_b) {
    // This method is allowed to look at constraints_.
    ZX_DEBUG_ASSERT(constraints_);

    int32_t cost_compare =
        UsagePixelFormatCost::Compare(parent_device_->pdev_device_info_vid(),
                                      parent_device_->pdev_device_info_pid(),
                                      constraints_.get(), index_a, index_b);
    if (cost_compare != 0) {
        return cost_compare;
    }

    // If we get this far, there's no known reason to choose one PixelFormat
    // over another, so just pick one based on a tie-breaker that'll distinguish
    // between PixelFormat(s).

    int32_t tie_breaker_compare = CompareImageFormatConstraintsTieBreaker(
        &constraints_->image_format_constraints[index_a],
        &constraints_->image_format_constraints[index_b]);
    return tie_breaker_compare;
}

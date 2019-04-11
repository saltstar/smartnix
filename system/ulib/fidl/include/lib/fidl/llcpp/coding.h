// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIB_FIDL_LLCPP_CODING_H_
#define LIB_FIDL_LLCPP_CODING_H_

#include <lib/fidl/llcpp/decoded_message.h>
#include <lib/fidl/llcpp/encoded_message.h>
#include <lib/fidl/llcpp/traits.h>

#ifdef __Fuchsia__
#include <zircon/syscalls.h>
#include <lib/zx/channel.h>
#endif

namespace fidl {

// The request/response type of any FIDL method with zero in/out parameters.
struct AnyZeroArgMessage {
    FIDL_ALIGNDECL
    fidl_message_header_t _hdr;

    static constexpr const fidl_type_t* Type = nullptr;
    static constexpr uint32_t MaxNumHandles = 0;
    static constexpr uint32_t PrimarySize = sizeof(fidl_message_header_t);
    static constexpr uint32_t MaxOutOfLine = 0;
};

template <>
struct IsFidlType<AnyZeroArgMessage> : public std::true_type {};
template <>
struct IsFidlMessage<AnyZeroArgMessage> : public std::true_type {};

template<typename FidlType>
struct DecodeResult {
    zx_status_t status = ZX_ERR_INTERNAL;
    const char* error = nullptr;
    DecodedMessage<FidlType> message;

    DecodeResult() = default;

    DecodeResult(zx_status_t status,
                 const char* error,
                 DecodedMessage<FidlType> message = DecodedMessage<FidlType>())
        : status(status), error(error), message(std::move(message)) {}
};

template<typename FidlType>
struct EncodeResult {
    zx_status_t status = ZX_ERR_INTERNAL;
    const char* error = nullptr;
    EncodedMessage<FidlType> message;

    EncodeResult() = default;

    EncodeResult(zx_status_t status,
                 const char* error,
                 EncodedMessage<FidlType> message = EncodedMessage<FidlType>())
        : status(status), error(error), message(std::move(message)) {}
};

template <typename FidlType>
struct LinearizeResult {
    zx_status_t status = ZX_ERR_INTERNAL;
    const char* error = nullptr;
    DecodedMessage<FidlType> message;

    LinearizeResult() = default;
};

// Consumes an encoded message object containing FIDL encoded bytes and handles.
// Uses the FIDL encoding tables to deserialize the message in-place.
// If the message is invalid, discards the buffer and returns an error.
template <typename FidlType>
DecodeResult<FidlType> Decode(EncodedMessage<FidlType> msg) {
    DecodeResult<FidlType> result;
    // Perform in-place decoding
    if (NeedsEncodeDecode<FidlType>::value) {
        result.status = fidl_decode(FidlType::Type,
                                    msg.bytes().data(), msg.bytes().actual(),
                                    msg.handles().data(), msg.handles().actual(),
                                    &result.error);
    } else {
        // Boring type does not need decoding
        if (msg.bytes().actual() != FidlType::PrimarySize) {
            result.error = "invalid size decoding";
        } else if (msg.handles().actual() != 0) {
            result.error = "invalid handle count decoding";
        } else {
            result.status = ZX_OK;
        }
    }
    // Clear out |msg| independent of success or failure
    BytePart bytes = msg.ReleaseBytesAndHandles();
    if (result.status == ZX_OK) {
        result.message.Reset(std::move(bytes));
    } else {
        result.message.Reset(BytePart());
    }
    return result;
}

// Serializes the content of the message in-place.
// The message's contents are always consumed by this operation, even in case of an error.
template <typename FidlType>
EncodeResult<FidlType> Encode(DecodedMessage<FidlType> msg) {
    EncodeResult<FidlType> result;
    result.status = result.message.Initialize([&msg, &result] (BytePart& msg_bytes,
                                                               HandlePart& msg_handles) {
        msg_bytes = std::move(msg.bytes_);
        if (NeedsEncodeDecode<FidlType>::value) {
            uint32_t actual_handles = 0;
            zx_status_t status = fidl_encode(FidlType::Type,
                                             msg_bytes.data(), msg_bytes.actual(),
                                             msg_handles.data(), msg_handles.capacity(),
                                             &actual_handles, &result.error);
            msg_handles.set_actual(actual_handles);
            return status;
        } else {
            // Boring type does not need encoding
            if (msg_bytes.actual() != FidlType::PrimarySize) {
                result.error = "invalid size encoding";
                return ZX_ERR_INVALID_ARGS;
            }
            msg_handles.set_actual(0);
            return ZX_OK;
        }
    });
    return result;
}

// Linearizes the contents of the message starting at |value|, into a continuous |bytes| buffer.
// Upon success, the handles in the source messages will be moved into |bytes|.
// the remaining contents in the source messages are otherwise untouched.
// In case of any failure, the handles from |value| will stay intact.
template <typename FidlType>
LinearizeResult<FidlType> Linearize(FidlType* value, BytePart bytes) {
    static_assert(IsFidlType<FidlType>::value, "FIDL type required");
    static_assert(FidlType::Type != nullptr, "FidlType should have a coding table");
    static_assert(FidlType::MaxOutOfLine > 0,
                  "Only types with out-of-line members need linearization");
    LinearizeResult<FidlType> result;
    uint32_t num_bytes_actual;
    result.status = fidl_linearize(FidlType::Type,
                                   value,
                                   bytes.data(),
                                   bytes.capacity(),
                                   &num_bytes_actual,
                                   &result.error);
    if (result.status != ZX_OK) {
        return result;
    }
    bytes.set_actual(num_bytes_actual);
    result.message = DecodedMessage<FidlType>(std::move(bytes));
    return result;
}

#ifdef __Fuchsia__

namespace {

template <bool, typename RequestType, typename ResponseType>
struct MaybeSelectResponseType {
    using type = ResponseType;
};

template <typename RequestType, typename ResponseType>
struct MaybeSelectResponseType<true, RequestType, ResponseType> {
    using type = typename RequestType::ResponseType;
};

} // namespace

// If |RequestType::ResponseType| exists, use that. Otherwise, fallback to |ResponseType|.
template <typename RequestType, typename ResponseType>
struct SelectResponseType {
    using type = typename MaybeSelectResponseType<internal::HasResponseType<RequestType>::value,
                                                  RequestType,
                                                  ResponseType>::type;
};

// Perform a synchronous FIDL channel call.
// Sends the request message down the channel, then waits for the desired reply message, and
// wraps it in an EncodeResult for the response type.
// If |RequestType| is |AnyZeroArgMessage|, the caller may explicitly specify an expected response
// type by overriding the template parameter |ResponseType|.
template <typename RequestType, typename ResponseType = typename RequestType::ResponseType>
EncodeResult<ResponseType> Call(zx::channel& chan,
                                EncodedMessage<RequestType> request,
                                BytePart response_buffer) {
    static_assert(IsFidlMessage<RequestType>::value, "FIDL transactional message type required");
    static_assert(IsFidlMessage<ResponseType>::value, "FIDL transactional message type required");
    // If |RequestType| has a defined |ResponseType|, ensure it matches the template parameter.
    static_assert(std::is_same<typename SelectResponseType<RequestType, ResponseType>::type,
                               ResponseType>::value,
                  "RequestType and ResponseType are incompatible");

    EncodeResult<ResponseType> result;
    result.message.Initialize([&](BytePart& bytes, HandlePart& handles) {
        bytes = std::move(response_buffer);
        zx_channel_call_args_t args = {
            .wr_bytes = request.bytes().data(),
            .wr_handles = request.handles().data(),
            .rd_bytes = bytes.data(),
            .rd_handles = handles.data(),
            .wr_num_bytes = request.bytes().actual(),
            .wr_num_handles = request.handles().actual(),
            .rd_num_bytes = bytes.capacity(),
            .rd_num_handles = handles.capacity()
        };

        uint32_t actual_num_bytes = 0u;
        uint32_t actual_num_handles = 0u;
        result.status = chan.call(
            0u, zx::time::infinite(), &args, &actual_num_bytes, &actual_num_handles);
        if (result.status != ZX_OK) {
            return;
        }

        bytes.set_actual(actual_num_bytes);
        handles.set_actual(actual_num_handles);
    });
    return result;
}

#endif

} // namespace fidl

#endif  // LIB_FIDL_LLCPP_CODING_H_

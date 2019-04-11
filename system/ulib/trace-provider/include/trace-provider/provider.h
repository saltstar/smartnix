// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//
// The API for initializing the trace provider for a process.
//

#pragma once

#include <zircon/compiler.h>
#include <zircon/types.h>

#include <lib/async/dispatcher.h>

__BEGIN_CDECLS

// The format of fifo packets for messages passed between the trace manager
// and trace providers.
typedef struct trace_provider_packet {
    // One of TRACE_PROVIDER_*.
    uint16_t request;

    // For alignment and future concerns, must be zero.
    uint16_t reserved;

    // Optional data for the request.
    // The contents depend on the request.
    // If unused they must be passed as zero.
    uint32_t data32;
    uint64_t data64;
} trace_provider_packet_t;

// The protocol version we are using.
// This is non-zero to catch initialization bugs.
#define TRACE_PROVIDER_FIFO_PROTOCOL_VERSION 1

// Provider->Manager
// Zero is reserved to catch initialization bugs.

// Indicate the provider successfully started.
// |data32| is TRACE_PROVIDER_FIFO_PROTOCOL_VERSION
// |data64| is unused (must be zero).
#define TRACE_PROVIDER_STARTED (0x1)

// Provider->Manager
// A buffer is full and needs to be saved (streaming mode only).
// |data32| is the "wrapped count", which is a count of the number of times
// a buffer has filled.
// |data64| is current offset in the durable buffer
#define TRACE_PROVIDER_SAVE_BUFFER (0x2)

// Temporary to ease soft-roll into garnet.
// Can be removed when garnet side lands.
#define TRACE_PROVIDER_BUFFER_OVERFLOW (0x2)

// Next Provider->Manager packet = 0x3

// Manager->Provider
// A buffer has been saved (streaminng mode only).
// |data32| is the "wrapped count", which is a count of the number of times
// a buffer has filled.
// |data64| is unused (must be zero).
#define TRACE_PROVIDER_BUFFER_SAVED (0x100)

// Next Manager->Provider packet = 0x101

// End fifo packet descriptions.

// Represents a trace provider.
typedef struct trace_provider trace_provider_t;

// Creates a trace provider associated with the specified async dispatcher
// and registers it with the tracing system.
//
// The trace provider will start and stop the trace engine in response to requests
// from the tracing system.
//
// |to_service| is the channel to use to connect to the trace manager.
// It is consumed regardless of success/failure.
//
// |dispatcher| is the asynchronous dispatcher which the trace provider and trace
// engine will use for dispatch.  This must outlive the trace provider instance.
//
// |name| is the name of the trace provider and is used for diagnostic
// purposes. The maximum supported length is 100 characters.
//
// Returns the trace provider, or null if creation failed.
//
// TODO(ZX-1036): Currently this connects to the trace manager service.
// Switch to passively exporting the trace provider via the "hub" through
// the process's exported directory once that stuff is implemented.  We'll
// probably need to pass some extra parameters to the trace provider then.
trace_provider_t* trace_provider_create_with_name_etc(
    zx_handle_t to_service, async_dispatcher_t* dispatcher, const char* name);

// Wrapper around trace_provider_create_with_name for backward compatibility.
// TODO(DX-422): Update all providers to use create_with_name, then change this
// to also take a name, then update all providers to call this one, and then
// delete trace_provider_create_with_name.
trace_provider_t* trace_provider_create_etc(
    zx_handle_t to_service, async_dispatcher_t* dispatcher);

// Same as trace_provider_create_with_name except does not return until the
// provider is registered with the trace manager.
// On return, if !NULL, |*out_already_started| is true if the trace manager has
// already started tracing, which is a hint to the provider to wait for the
// Start() message before continuing if it wishes to not drop trace records
// before Start() is received.
trace_provider_t* trace_provider_create_synchronously_etc(
    zx_handle_t to_service, async_dispatcher_t* dispatcher, const char* name,
    bool* out_already_started);

// Wrappers on the above functions that use fdio.
trace_provider_t* trace_provider_create_with_name_fdio(
    async_dispatcher_t* dispatcher, const char* name);
trace_provider_t* trace_provider_create_with_fdio(
    async_dispatcher_t* dispatcher);
trace_provider_t* trace_provider_create_synchronously_with_fdio(
    async_dispatcher_t* dispatcher, const char* name,
    bool* out_already_started);

// Compatibility wrappers.
// TODO(PT-63): Remove these (and the _etc suffixes on the above routines)
// when all clients are updated.
trace_provider_t* trace_provider_create_with_name(
    async_dispatcher_t* dispatcher, const char* name);
trace_provider_t* trace_provider_create(async_dispatcher_t* dispatcher);
trace_provider_t* trace_provider_create_synchronously(
    async_dispatcher_t* dispatcher, const char* name,
    bool* out_already_started);

// Destroys the trace provider.
void trace_provider_destroy(trace_provider_t* provider);

__END_CDECLS

#ifdef __cplusplus

#include <fbl/unique_ptr.h>
#include <lib/zx/channel.h>

namespace trace {

// Convenience RAII wrapper for creating and destroying a trace provider.
// TODO(PT-63): Remove Etc suffix when all clients are updated.
class TraceProviderEtc {
public:
    // Create a trace provider synchronously, and return an indicator of
    // whether tracing has started already in |*out_already_started|.
    // Returns a boolean indicating success.
    // This is done with a factory function because it's more complex than
    // the basic constructor.
    static bool CreateSynchronously(
            zx::channel to_service,
            async_dispatcher_t* dispatcher,
            const char* name,
            fbl::unique_ptr<TraceProviderEtc>* out_provider,
            bool* out_already_started) {
        auto provider = trace_provider_create_synchronously_etc(
            to_service.release(), dispatcher, name, out_already_started);
        if (!provider)
            return false;
        *out_provider = fbl::unique_ptr<TraceProviderEtc>(
            new TraceProviderEtc(provider));
        return true;
    }

    // Creates a trace provider.
    TraceProviderEtc(zx::channel to_service, async_dispatcher_t* dispatcher)
        : provider_(trace_provider_create_etc(to_service.release(),
                                              dispatcher)) {}

    // Creates a trace provider.
    TraceProviderEtc(zx::channel to_service, async_dispatcher_t* dispatcher,
                     const char* name)
        : provider_(trace_provider_create_with_name_etc(to_service.release(),
                                                        dispatcher, name)) {}

    // Destroys a trace provider.
    ~TraceProviderEtc() {
        if (provider_)
            trace_provider_destroy(provider_);
    }

    // Returns true if the trace provider was created successfully.
    bool is_valid() const {
        return provider_ != nullptr;
    }

protected:
    explicit TraceProviderEtc(trace_provider_t* provider)
        : provider_(provider) {}

private:
    trace_provider_t* const provider_;
};

class TraceProviderWithFdio : public TraceProviderEtc {
public:
    static bool CreateSynchronously(
            async_dispatcher_t* dispatcher,
            const char* name,
            fbl::unique_ptr<TraceProviderWithFdio>* out_provider,
            bool* out_already_started) {
        auto provider = trace_provider_create_synchronously_with_fdio(
            dispatcher, name, out_already_started);
        if (!provider)
            return false;
        *out_provider = fbl::unique_ptr<TraceProviderWithFdio>(
            new TraceProviderWithFdio(provider));
        return true;
    }

    // Creates a trace provider.
    explicit TraceProviderWithFdio(async_dispatcher_t* dispatcher)
        : TraceProviderWithFdio(
            trace_provider_create_with_fdio(dispatcher)) {}

    // Creates a trace provider.
    explicit TraceProviderWithFdio(async_dispatcher_t* dispatcher,
                                   const char* name)
        : TraceProviderWithFdio(
            trace_provider_create_with_name_fdio(dispatcher, name)) {}

private:
    explicit TraceProviderWithFdio(trace_provider_t* provider)
        : TraceProviderEtc(provider) {}
};

// Compatibility wrapper.
// TODO(PT-63): Delete (and remove _etc from above version) when all clients
// are updated.
using TraceProvider = TraceProviderWithFdio;

} // namespace trace

#endif // __cplusplus

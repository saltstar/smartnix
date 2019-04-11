// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stdint.h>
#include <unistd.h>

#include <cobalt-client/cpp/metric-options.h>
#include <cobalt-client/cpp/types-internal.h>

#include <fbl/function.h>
#include <fbl/string.h>
#include <fbl/vector.h>
#ifdef __Fuchsia__
#include <lib/zx/time.h>
#include <lib/zx/vmo.h>
#endif

#include <atomic>
#include <memory>

namespace cobalt_client {

// Defines the options for initializing the Collector.
struct CollectorOptions {
    // Returns a |Collector| whose data will be logged for GA release stage.
    static CollectorOptions GeneralAvailability();

    // Returns a |Collector| whose data will be logged for Dogfood release stage.
    static CollectorOptions Dogfood();

    // Returns a |Collector| whose data will be logged for Fishfood release stage.
    static CollectorOptions Fishfood();

    // Returns a |Collector| whose data will be logged for Debug release stage.
    static CollectorOptions Debug();

#ifdef __Fuchsia__
    // Callback used when reading the config to create a cobalt logger.
    // Returns true when the write was successful. The VMO will be transferred
    // to the cobalt service.
    fbl::Function<bool(zx::vmo*, size_t*)> load_config = nullptr;

    // Configuration for RPC behavior for remote metrics.
    // Only set if you plan to interact with cobalt service.

    // When registering with cobalt, will block for this amount of time, each
    // time we need to reach cobalt, until the response is received.
    zx::duration response_deadline = zx::duration(0);

    // When registering with cobalt, will block for this amount of time, the first
    // time we need to wait for a response.
    zx::duration initial_response_deadline = zx::duration(0);

    // The Id of the cobalt project to use. This is a registered project in Cobalt's metric
    // registry. If this value is -1, then |load_config| must be set.
    int64_t project_id = -1;
#endif
    // This is set internally by factory functions.
    uint32_t release_stage = 0;
};

// This class acts as a peer for instantiating Histograms and Counters. All
// objects instantiated through this class act as a view, which means that
// their lifetime is coupled to this object's lifetime. This class does require
// the number of different configurations on construction.
//
// The Sink provides an API for persisting the supported data types. This is
// exposed to simplify testing.
//
// This class is not moveable, copyable or assignable.
// This class is thread-compatible.
class Collector {
public:
    Collector(CollectorOptions options);
    Collector(std::unique_ptr<internal::Logger> logger);
    Collector(const Collector&) = delete;
    Collector(Collector&&) = delete;
    Collector& operator=(const Collector&) = delete;
    Collector& operator=(Collector&&) = delete;
    ~Collector();

    // Allows classes implementing |internal::FlushInterface| to subscribe for Flush events.
    void Subscribe(internal::FlushInterface* flushable) { flushables_.push_back(flushable); }

    // Allows classes implementing |internal::FlushInterface| to UnSubscribe for Flush events.
    void UnSubscribe(internal::FlushInterface* flushable);

    // Flushes the content of all flushable metrics into |logger_|. The |logger_| is
    // in charge of persisting the data.
    void Flush();

private:
    // Convert this into a HashTable.
    fbl::Vector<internal::FlushInterface*> flushables_;

    std::unique_ptr<internal::Logger> logger_ = nullptr;
    std::atomic<bool> flushing_ = false;
};

} // namespace cobalt_client

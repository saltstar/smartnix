// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stdint.h>

#include <cobalt-client/cpp/collector.h>
#include <cobalt-client/cpp/counter-internal.h>
#include <cobalt-client/cpp/types-internal.h>

namespace cobalt_client {

// Thin wrapper for an atomic counter with a fixed memory order. The counter handles
// a remote count and a local count. The remote count is periodically flushed, while
// the local count is viewed on demand (and optionally flushed depending on configuration).
//
// This class is copyable, moveable and assignable.
// This class is thread-safe.
class Counter {
public:
    // Underlying type used for representing an actual counter.
    using Count = uint64_t;

    Counter() = default;
    Counter(const MetricOptions& options);
    Counter(const MetricOptions& options, Collector* collector);
    // Constructor for internal use only.
    Counter(const MetricOptions& options, internal::FlushInterface** flush_interface);
    Counter(const Counter& other) = delete;
    Counter(Counter&&) = delete;
    ~Counter();

    // Optionally initialize lazily the histogram, if is more readable to do so
    // in the constructor or function body.
    void Initialize(const MetricOptions& options, Collector* collector);

    // Increments the counter value by |value|. This applies to local and remote
    // values of the counter.
    void Increment(Count value = 1);

    // Returns the current value of the counter that would be
    // sent to the remote service(cobalt).
    Count GetRemoteCount() const;

private:
    internal::RemoteCounter remote_counter_;
    Collector* collector_ = nullptr;
    MetricOptions::Mode mode_ = MetricOptions::Mode::kLazy;
};

} // namespace cobalt_client

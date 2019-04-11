// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <limits.h>
#include <stdint.h>

#include <cobalt-client/cpp/counter-internal.h>
#include <cobalt-client/cpp/histogram-internal.h>
#include <cobalt-client/cpp/types-internal.h>
#include <fbl/string_buffer.h>
#include <lib/zx/channel.h>
#include <lib/zx/time.h>
#include <lib/zx/vmo.h>
#include <zircon/types.h>

namespace cobalt_client {
namespace internal {

struct CobaltOptions {
    // Service path to LoggerFactory interface.
    fbl::StringBuffer<PATH_MAX> service_path;

    // Maximum time to wait for Cobalt Service to respond for the CreateLogger request.
    // Unless the channel is closed, we will keep checking if the channel is readable.
    zx::duration logger_deadline;

    // The maximum time to wait, after the request has been written to the channel.
    // This allows amortizing the wait time in future calls.
    zx::duration logger_deadline_first_attempt;

    // Sets the input VMO to point to the serialized config for this logger and the size
    // of the serialized data.
    fbl::Function<bool(zx::vmo*, size_t*)> config_reader;

    // Performs a connection to a service at a given path.
    fbl::Function<zx_status_t(const char* service_path, zx::channel service)> service_connect;

    // The Id of the cobalt project to use. This is a registered project in Cobalt's metric
    // registry. If this value is -1, then config reader must be set.
    int64_t project_id = -1;

    // Which release stage to use for persisting metrics.
    ReleaseStage release_stage;
};

class CobaltLogger : public Logger {
public:
    CobaltLogger() = delete;
    // instance from cobalt service;
    explicit CobaltLogger(CobaltOptions options);
    CobaltLogger(const CobaltLogger&) = delete;
    CobaltLogger(CobaltLogger&&) = delete;
    CobaltLogger& operator=(const CobaltLogger&) = delete;
    CobaltLogger& operator=(CobaltLogger&&) = delete;
    ~CobaltLogger() override{};

    // Returns true if the histogram was persisted.
    bool Log(const RemoteMetricInfo& metric_info, const HistogramBucket* buckets,
             size_t bucket_count) override;

    // Returns true if the counter was persisted.
    bool Log(const RemoteMetricInfo& metric_info, int64_t count) override;

    bool IsListeningForReply() const { return logger_factory_.is_valid(); }

    // Blocks until the reply from LoggerFactory arrives into |logger_factory_|
    // channel or the peer is closed. |observed| will be set to the observed signals
    // if provided. Useful for testing to enforce a deterministic order of operations.
    zx_status_t WaitForReply(zx_signals_t* observed = nullptr) const {
        return zx_object_wait_one(logger_factory_.get(),
                                  ZX_CHANNEL_READABLE | ZX_CHANNEL_PEER_CLOSED,
                                  zx::time::infinite().get(), observed);
    }

protected:
    // If returns true, a channel has been established with the endpoint,
    // and the handshake to set up a logger started.
    bool TrySendLoggerRequest();

    // The service replied and the status is ok.
    bool HasCobaltReplied(zx::duration deadline);

    // Returns true if the logger request has been sent, and Cobalt Service
    // replied successfully already. If any error happens that prevents
    // writing to the current channel(ZX_ERR_PEER_CLOSED), we guarantee
    // the next time this method is called will return false.
    bool IsLoggerReady();

    // Set of options for this logger.
    CobaltOptions options_;

    zx::channel logger_;
    zx::channel logger_factory_;

    bool is_first_attempt_;
};

} // namespace internal
} // namespace cobalt_client

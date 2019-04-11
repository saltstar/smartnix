// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIB_ZXIO_OPS_H_
#define LIB_ZXIO_OPS_H_

#include <lib/zxio/types.h>
#include <lib/zxio/zxio.h>
#include <stdint.h>
#include <zircon/compiler.h>
#include <zircon/types.h>

__BEGIN_CDECLS

// See zxio.h
typedef struct zxio {
    uint64_t reserved[4];
} zxio_t;

// Storage for the |zxio_ops_t| implementation.
typedef struct zxio_private {
    uint64_t reserved[6];
} zxio_private_t;

// The storage backing a |zxio_t|.
typedef struct zxio_storage {
    zxio_t io;
    zxio_private_t reserved;
} zxio_storage_t;

// A table of operations for a zxio_t.
//
// Most of the functions that operate on a zxio_t call through this operations
// table to actually perform the operation. Use |zxio_alloc| to create a zxio_t
// with a custom operations table.
typedef struct zxio_ops zxio_ops_t;

// Initialize a |zxio_t| object with the given |ops| table.
void zxio_init(zxio_t* io, const zxio_ops_t* ops);

struct zxio_ops {
    // After |release| returns, no further ops will be called relative to |ctx|.
    zx_status_t (*release)(zxio_t* io, zx_handle_t* out_handle);

    // After |close| returns, no further ops will be called relative to |ctx|.
    zx_status_t (*close)(zxio_t* io);

    void (*wait_begin)(zxio_t* io, zxio_signals_t zxio_signals,
                       zx_handle_t* out_handle, zx_signals_t* out_zx_signals);
    void (*wait_end)(zxio_t* io, zx_signals_t zx_signals,
                     zxio_signals_t* out_zxio_signals);
    zx_status_t (*clone_async)(zxio_t* io, uint32_t flags, zx_handle_t request);
    zx_status_t (*sync)(zxio_t* io);
    zx_status_t (*attr_get)(zxio_t* io, zxio_node_attr_t* out_attr);
    zx_status_t (*attr_set)(zxio_t* io, uint32_t flags,
                            const zxio_node_attr_t* attr);
    zx_status_t (*read)(zxio_t* io, void* buffer, size_t capacity,
                        size_t* out_actual);
    zx_status_t (*read_at)(zxio_t* io, size_t offset, void* buffer,
                           size_t capacity, size_t* out_actual);
    zx_status_t (*write)(zxio_t* io, const void* buffer, size_t capacity,
                         size_t* out_actual);
    zx_status_t (*write_at)(zxio_t* io, size_t offset, const void* buffer,
                            size_t capacity, size_t* out_actual);
    zx_status_t (*seek)(zxio_t* io, size_t offset, zxio_seek_origin_t start,
                        size_t* out_offset);
    zx_status_t (*truncate)(zxio_t* io, size_t length);
    zx_status_t (*flags_get)(zxio_t* io, uint32_t* out_flags);
    zx_status_t (*flags_set)(zxio_t* io, uint32_t flags);
    zx_status_t (*vmo_get)(zxio_t* io, uint32_t flags, zx_handle_t* out_vmo,
                           size_t* out_size);
    zx_status_t (*open)(zxio_t* io, uint32_t flags, uint32_t mode,
                        const char* path, zxio_t** out_io);
    zx_status_t (*open_async)(zxio_t* io, uint32_t flags, uint32_t mode,
                              const char* path, zx_handle_t request);
    zx_status_t (*unlink)(zxio_t* io, const char* path);
    zx_status_t (*token_get)(zxio_t* io, zx_handle_t* out_token);
    zx_status_t (*rename)(zxio_t* io, const char* src_path,
                          zx_handle_t dst_token, const char* dst_path);
    zx_status_t (*link)(zxio_t* io, const char* src_path, zx_handle_t dst_token,
                        const char* dst_path);
    zx_status_t (*readdir)(zxio_t* io, void* buffer, size_t capacity,
                           size_t* out_actual);
    zx_status_t (*rewind)(zxio_t* io);
};

__END_CDECLS

#endif // LIB_ZXIO_OPS_H_

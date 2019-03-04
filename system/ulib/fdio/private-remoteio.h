
#pragma once

#include <lib/zxio/inception.h>

#include "private.h"

// Implements the |fdio_t| contract using |zxio_remote_t|.
//
// Has an ops table that translates fdio ops into zxio ops. Some of the fdio ops
// require using the underlying handles in the |zxio_remote_t|, which is why
// this object needs to use |zxio_remote_t| directly.
//
// Will be removed once the transition to the zxio backend is complete.
typedef struct fdio_zxio_remote {
    fdio_t io;
    zxio_remote_t remote;
} fdio_zxio_remote_t;

// Create an |fdio_t| for a remote file backed by zxio.
fdio_t* fdio_zxio_create_remote(zx_handle_t control, zx_handle_t event);

// open operation directly on remoteio handle
zx_status_t zxrio_open_handle(zx_handle_t h, const char* path, uint32_t flags,
                              uint32_t mode, fdio_t** out);

extern fdio_ops_t fdio_zxio_remote_ops;

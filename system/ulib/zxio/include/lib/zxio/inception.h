
#ifndef LIB_ZXIO_INCEPTION_H_
#define LIB_ZXIO_INCEPTION_H_

#include <lib/zxio/ops.h>
#include <zircon/compiler.h>
#include <zircon/types.h>

// This header exposes some guts of zxio in order to transition fdio to build on
// top of zxio.

__BEGIN_CDECLS

// null ------------------------------------------------------------------------

zx_status_t zxio_null_init(zxio_t* remote);

// remote ----------------------------------------------------------------------

// A |zxio_t| backend that uses the |fuchsia.io.Node| protocol.
//
// The |control| handle is a channel that implements the |fuchsia.io.Node|. The
// |event| handle is an optional event object used with some |fuchsia.io.Node|
// servers.
//
// Will eventually be an implementation detail of zxio once fdio completes its
// transition to the zxio backend.
typedef struct zxio_remote {
    zxio_t io;
    zx_handle_t control;
    zx_handle_t event;
} zxio_remote_t;

zx_status_t zxio_remote_init(zxio_remote_t* remote, zx_handle_t control,
                             zx_handle_t event);

// vmofile ---------------------------------------------------------------------

typedef struct zxio_vmofile {
    zxio_t io;
    zx_handle_t control;
    zx_handle_t vmo;
    // etc
} zxio_vmofile_t;

// pipe ------------------------------------------------------------------------

// A |zxio_t| backend that uses a Zircon socket object.
//
// The |socket| handle is a Zircon socket object.
//
// Will eventually be an implementation detail of zxio once fdio completes its
// transition to the zxio backend.
typedef struct zxio_pipe {
    zxio_t io;
    zx_handle_t socket;
} zxio_pipe_t;

zx_status_t zxio_pipe_init(zxio_pipe_t* pipe, zx_handle_t socket);

__END_CDECLS

#endif // LIB_ZXIO_INCEPTION_H_


#include <lib/zxio/inception.h>
#include <lib/zxio/null.h>
#include <zircon/syscalls.h>

zx_status_t zxio_null_release(zxio_t* io, zx_handle_t* out_handle) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_close(zxio_t* io) {
    return ZX_ERR_NOT_SUPPORTED;
}

void zxio_null_wait_begin(zxio_t* io, zxio_signals_t zxio_signals,
                          zx_handle_t* out_handle,
                          zx_signals_t* out_zx_signals) {
    *out_handle = ZX_HANDLE_INVALID;
    *out_zx_signals = ZX_SIGNAL_NONE;
}

void zxio_null_wait_end(zxio_t* io, zx_signals_t zx_signals,
                    zxio_signals_t* out_zxio_signals) {
    *out_zxio_signals = ZXIO_SIGNAL_NONE;
}

zx_status_t zxio_null_clone_async(zxio_t* io, uint32_t flags,
                                  zx_handle_t request) {
    zx_handle_close(request);
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_sync(zxio_t* io) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_attr_get(zxio_t* io, zxio_node_attr_t* out_attr) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_attr_set(zxio_t* io, uint32_t flags,
                               const zxio_node_attr_t* attr) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_read(zxio_t* io, void* buffer, size_t capacity,
                           size_t* out_actual) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_read_at(zxio_t* io, size_t offset, void* buffer,
                              size_t capacity, size_t* out_actual) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_write(zxio_t* io, const void* buffer, size_t capacity,
                            size_t* out_actual) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_write_at(zxio_t* io, size_t offset, const void* buffer,
                               size_t capacity, size_t* out_actual) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_seek(zxio_t* io, size_t offset, zxio_seek_origin_t start,
                           size_t* out_offset) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_truncate(zxio_t* io, size_t length) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_flags_get(zxio_t* io, uint32_t* out_flags) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_flags_set(zxio_t* io, uint32_t flags) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_vmo_get(zxio_t* io, uint32_t flags, zx_handle_t* out_vmo,
                              size_t* out_size) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_open(zxio_t* io, uint32_t flags, uint32_t mode,
                           const char* path, zxio_t** out_io) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_open_async(zxio_t* io, uint32_t flags, uint32_t mode,
                                 const char* path, zx_handle_t request) {
    zx_handle_close(request);
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_unlink(zxio_t* io, const char* path) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_token_get(zxio_t* io, zx_handle_t* out_token) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_rename(zxio_t* io, const char* src_path,
                             zx_handle_t dst_token, const char* dst_path) {
    zx_handle_close(dst_token);
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_link(zxio_t* io, const char* src_path,
                           zx_handle_t dst_token, const char* dst_path) {
    zx_handle_close(dst_token);
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_readdir(zxio_t* io, void* buffer, size_t capacity,
                              size_t* out_actual) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zxio_null_rewind(zxio_t* io) {
    return ZX_ERR_NOT_SUPPORTED;
}

static const zxio_ops_t zxio_null_ops = {
    .release = zxio_null_release,
    .close = zxio_null_close,
    .wait_begin = zxio_null_wait_begin,
    .wait_end = zxio_null_wait_end,
    .clone_async = zxio_null_clone_async,
    .sync = zxio_null_sync,
    .attr_get = zxio_null_attr_get,
    .attr_set = zxio_null_attr_set,
    .read = zxio_null_read,
    .read_at = zxio_null_read_at,
    .write = zxio_null_write,
    .write_at = zxio_null_write_at,
    .seek = zxio_null_seek,
    .truncate = zxio_null_truncate,
    .flags_get = zxio_null_flags_get,
    .flags_set = zxio_null_flags_set,
    .vmo_get = zxio_null_vmo_get,
    .open = zxio_null_open,
    .open_async = zxio_null_open_async,
    .unlink = zxio_null_unlink,
    .token_get = zxio_null_token_get,
    .rename = zxio_null_rename,
    .link = zxio_null_link,
    .readdir = zxio_null_readdir,
    .rewind = zxio_null_rewind,
};

zx_status_t zxio_null_init(zxio_t* io) {
    zxio_init(io, &zxio_null_ops);
    return ZX_OK;
}

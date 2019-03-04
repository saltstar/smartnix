
#include "xdc-init.h"

#include <xdc-host-utils/client.h>

#include <stdio.h>

zx_status_t configure_xdc(const uint32_t stream_id, fbl::unique_fd* out_fd) {
    zx_status_t status = xdc::GetStream(stream_id, out_fd);
    if (status != ZX_OK) {
        return status;
    }
    printf("client has fd %d, stream id %u\n", out_fd->get(), stream_id);
    return ZX_OK;
}

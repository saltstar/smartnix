
#include <ddk/device.h>
#include <ddk/driver.h>
#include <ddk/binding.h>

#include <zircon/types.h>

extern zx_status_t ddktl_test_bind(void* ctx, zx_device_t* dev);

static zx_driver_ops_t ddktl_test_driver_ops = {
    .version = DRIVER_OPS_VERSION,
    .bind = ddktl_test_bind,
};

ZIRCON_DRIVER_BEGIN(ddktl_test, ddktl_test_driver_ops, "zircon", "0.1", 2)
    BI_ABORT_IF_AUTOBIND,
    BI_MATCH_IF(EQ, BIND_PROTOCOL, ZX_PROTOCOL_TEST),
ZIRCON_DRIVER_END(ddktl_test)

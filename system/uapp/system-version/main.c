
#include <assert.h>
#include <stdio.h>
#include <zircon/syscalls.h>

int main(int argc, char** argv) {
    char buf[64];
    zx_status_t status = zx_system_get_version(buf, sizeof(buf));
    assert(status == ZX_OK);
    printf("%s\n", buf);
    return 0;
}

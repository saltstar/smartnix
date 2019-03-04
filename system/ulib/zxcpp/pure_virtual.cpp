
#include <zircon/assert.h>

extern "C" void __cxa_pure_virtual(void) {
    ZX_PANIC("pure virtual called\n");
}


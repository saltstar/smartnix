
#include <debug.h>
#include <err.h>
#include <platform.h>

/*
 * default implementations of these routines, if the platform code
 * chooses not to implement.
 */

__WEAK void platform_init_mmu_mappings() {
}

__WEAK void platform_early_init() {
}

__WEAK void platform_init() {
}

__WEAK void platform_quiesce() {
}

__WEAK void platform_panic_start() {
}

__WEAK void* platform_get_ramdisk(size_t* size) {
    *size = 0;
    return NULL;
}

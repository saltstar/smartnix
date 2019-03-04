
#include <debug.h>
#include <err.h>
#include <kernel/thread.h>
#include <lib/console.h>
#include <platform.h>
#include <platform/debug.h>
#include <stdio.h>
#include <zircon/compiler.h>

/*
 * default implementations of these routines, if the platform code
 * chooses not to implement.
 */
__WEAK void platform_halt(platform_halt_action suggested_action,
                          platform_halt_reason reason) {

    thread_print_current_backtrace();

#if ENABLE_PANIC_SHELL
    if (reason == HALT_REASON_SW_PANIC) {
        dprintf(ALWAYS, "CRASH: starting debug shell... (reason = %d)\n", reason);
        arch_disable_ints();
        panic_shell_start();
    }
#endif // ENABLE_PANIC_SHELL

    dprintf(ALWAYS, "HALT: spinning forever... (reason = %d)\n", reason);
    arch_disable_ints();
    for (;;) {
    }
}

__WEAK void platform_halt_cpu() {
}

__WEAK void platform_halt_secondary_cpus() {
    PANIC_UNIMPLEMENTED;
}


#include <kernel/init.h>

#include <debug.h>
#include <kernel/mp.h>
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <zircon/compiler.h>

void kernel_init(void) {
    dprintf(SPEW, "initializing mp\n");
    mp_init();

    dprintf(SPEW, "initializing timers\n");
    timer_queue_init();
}

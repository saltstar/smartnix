
#include <lib/async/default.h>

#include <threads.h>

static thread_local async_dispatcher_t* g_default;

async_dispatcher_t* async_get_default_dispatcher(void) {
    return g_default;
}

void async_set_default_dispatcher(async_dispatcher_t* dispatcher) {
    g_default = dispatcher;
}

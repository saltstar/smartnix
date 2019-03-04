
#pragma once

#include <zircon/types.h>

// Register the devhost as a "trace provider" with the trace manager.
// There is no corresponding "unregister" function: we remain registered
// until either us or the manager terminate.
zx_status_t devhost_start_trace_provider();

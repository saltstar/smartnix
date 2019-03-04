
#pragma once

#pragma GCC visibility push(hidden)

#include <zircon/types.h>
#include <stddef.h>

struct bootfs;

// Handle loader-service RPCs on channel until there are no more.
// Consumes the channel.
void loader_service(zx_handle_t log, struct bootfs*, zx_handle_t channel);

#pragma GCC visibility pop

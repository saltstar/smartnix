
#pragma once

#include <kernel/thread.h>

// These are the used tls entries for thread_t's tls_get(), tls_set()
// and tls_set_callback(). Add entries here up to THREAD_MAX_TLS_ENTRY - 1.

#define TLS_ENTRY_KOBJ_DELETER      0
#define TLS_ENTRY_LAST              1

static_assert(TLS_ENTRY_LAST <= (THREAD_MAX_TLS_ENTRY - 1), "");

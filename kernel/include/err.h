
#pragma once

#ifndef __ASSEMBLER__
#include <zircon/types.h> // for zx_status_t
#endif

#include <zircon/errors.h>

// TODO: This is used primarily by class drivers which are obsolete.
// Re-examine when those are removed.
#define ZX_ERR_NOT_CONFIGURED (-501)

// MOVE to kernel internal used for thread teardown
#define ZX_ERR_INTERNAL_INTR_KILLED (-502)

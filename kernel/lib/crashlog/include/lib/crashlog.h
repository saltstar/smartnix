
#pragma once

#if defined(__aarch64__)

#include <arch/arm64.h>

#elif defined(__x86_64__)

#include <arch/x86.h>

#endif

typedef struct {
    uintptr_t base_address;
    iframe_t* iframe;
} crashlog_t;

extern crashlog_t crashlog;

// Serialize the crashlog to string in `out' up to `len' characters.
size_t crashlog_to_string(char* out, size_t len);

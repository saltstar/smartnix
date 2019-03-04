
#pragma once

#include <sys/types.h>
#include <stdbool.h>
#include <stdarg.h>
#include <zircon/compiler.h>

__BEGIN_CDECLS

void platform_debug_panic_start(void);
void platform_dputs_thread(const char* str, size_t len);
void platform_dputs_irq(const char* str, size_t len);
int platform_dgetc(char *c, bool wait);
static inline void platform_dputc(char c) {
    platform_dputs_thread(&c, 1);
}

// Should be available even if the system has panicked.
void platform_pputc(char c);
int platform_pgetc(char *c, bool wait);

__END_CDECLS

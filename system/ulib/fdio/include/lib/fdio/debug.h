
#pragma once

#include <stdio.h>
#include <zircon/compiler.h>

__BEGIN_CDECLS

// per-file chatty debug macro
#define xprintf(fmt, args...)                                                                      \
    do {                                                                                           \
        if (ZXDEBUG) {                                                                             \
            printf("%s:%d: " fmt, __FILE__, __LINE__, ##args);                                     \
        }                                                                                          \
    } while (0)

__END_CDECLS

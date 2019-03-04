
#pragma once

#include <dev/uart.h>
#include <zircon/compiler.h>

__BEGIN_CDECLS

// UART interface
struct pdev_uart_ops {
    int (*getc)(bool wait);

    /* panic-time uart accessors, intended to be run with interrupts disabled */
    int (*pputc)(char c);
    int (*pgetc)(void);

    void (*start_panic)(void);
    void (*dputs)(const char* str, size_t len, bool block, bool map_NL);
};

void pdev_register_uart(const struct pdev_uart_ops* ops);

__END_CDECLS

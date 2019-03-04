
#include <stdint.h>
#include "debug.h"

#define UART_THR                    (0x0)   // TX Buffer Register (write-only)
#define UART_LSR                    (0x14)  // Line Status Register
#define UART_LSR_THRE               (1 << 5)

#define UARTREG(reg) (*(volatile uint32_t*)(0x11005000 + (reg)))

void uart_pputc(char c) {
    while (!(UARTREG(UART_LSR) & UART_LSR_THRE))
        ;
    UARTREG(UART_THR) = c;
}

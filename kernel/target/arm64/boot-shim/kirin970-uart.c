
#include <stdint.h>
#include "debug.h"

static volatile uint32_t* uart_fifo_dr = (uint32_t *)0xfff32000;
static volatile uint32_t* uart_fifo_fr = (uint32_t *)0xfff32018;

void uart_pputc(char c) {
    /* spin while fifo is full */
    while (*uart_fifo_fr & (1<<5))
        ;
    *uart_fifo_dr = c;
}

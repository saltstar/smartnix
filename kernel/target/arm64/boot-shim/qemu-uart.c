
#include <stdint.h>
#include "debug.h"

static volatile uint32_t* uart_fifo_dr = (uint32_t *)0x09000000;
static volatile uint32_t* uart_fifo_fr = (uint32_t *)0x09000018;

void uart_pputc(char c) {
    /* spin while fifo is full */
    while (*uart_fifo_fr & (1<<5))
        ;
    *uart_fifo_dr = c;
}

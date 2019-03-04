
#pragma once

#include <stdint.h>

// Uncomment to enable debug UART.
// #define DEBUG_UART 1

// Board specific.
void uart_pputc(char c);

// Common code.
void uart_puts(const char* str);
void uart_print_hex(uint64_t value);

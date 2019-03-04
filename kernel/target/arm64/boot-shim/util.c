
#include "debug.h"
#include "util.h"

void fail(const char* message) {
    uart_puts(message);
    while (1) {}
}

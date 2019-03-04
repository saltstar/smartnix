
#pragma once

#include <lib/cbuf.h>

void platform_init_keyboard(cbuf_t* buffer);

int platform_read_key(char* c);

// Reboot the system via the keyboard, returns on failure.
void pc_keyboard_reboot(void);


#pragma once

#include <zircon/compiler.h>

__BEGIN_CDECLS

enum reboot_flags {
    REBOOT_NORMAL = 0,
    REBOOT_BOOTLOADER = 1,
    REBOOT_RECOVERY = 2,
};

void power_reboot(enum reboot_flags flags);
void power_shutdown(void);

__END_CDECLS

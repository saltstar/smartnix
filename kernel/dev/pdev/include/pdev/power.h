
#pragma once

#include <dev/power.h>
#include <zircon/compiler.h>

__BEGIN_CDECLS

// power interface
struct pdev_power_ops {
    void (*reboot)(enum reboot_flags flags);
    void (*shutdown)(void);
};

void pdev_register_power(const struct pdev_power_ops* ops);

__END_CDECLS

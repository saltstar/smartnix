
#include <pdev/power.h>

#include <arch/arch_ops.h>
#include <dev/power.h>
#include <dev/psci.h>

static void default_reboot(enum reboot_flags flags) {
    psci_system_reset(flags);
}

static void default_shutdown() {
    psci_system_off();
}

static const struct pdev_power_ops default_ops = {
    .reboot = default_reboot,
    .shutdown = default_shutdown,
};

static const struct pdev_power_ops* power_ops = &default_ops;

void power_reboot(enum reboot_flags flags) {
    power_ops->reboot(flags);
}

void power_shutdown() {
    power_ops->shutdown();
}

void pdev_register_power(const struct pdev_power_ops* ops) {
    power_ops = ops;
    smp_mb();
}

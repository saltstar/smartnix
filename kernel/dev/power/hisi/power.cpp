
#include <arch/arm64/periphmap.h>
#include <dev/power.h>
#include <dev/psci.h>
#include <pdev/driver.h>
#include <pdev/power.h>
#include <zircon/boot/driver-config.h>
#include <stdio.h>

// clang-format off
#define PMU_HRST_OFFSET         0x404
#define SCTRL_PEREN1_OFFSET     0x170
#define SCTRL_DDR_BYPASS        (1 << 31)
#define SCTRL_REBOOT_OFFSET     0x4
// clang-format on

static vaddr_t sctrl_base;
static vaddr_t pmu_base;

static void hisi_reboot(enum reboot_flags flags) {
    uint32_t temp = readl(pmu_base + PMU_HRST_OFFSET);
    temp = (temp & 0xFFFFFF00) | (flags == REBOOT_BOOTLOADER ? 1 : 0);
    writel(temp, pmu_base + PMU_HRST_OFFSET);

    writel(SCTRL_DDR_BYPASS, sctrl_base + SCTRL_PEREN1_OFFSET);
    writel(0xdeadbeef, sctrl_base + SCTRL_REBOOT_OFFSET);
}

static void hisi_shutdown() {
    printf("SHUTDOWN NOT IMPLEMENTED\n");
}

static const struct pdev_power_ops hisi_power_ops = {
    .reboot = hisi_reboot,
    .shutdown = hisi_shutdown,
};

static void hisi_power_init(const void* driver_data, uint32_t length) {
    ASSERT(length >= sizeof(dcfg_hisilicon_power_driver_t));
    auto driver = static_cast<const dcfg_hisilicon_power_driver_t*>(driver_data);
    ASSERT(driver->sctrl_phys && driver->pmu_phys);


    // get virtual addresses of our peripheral bases
    sctrl_base = periph_paddr_to_vaddr(driver->sctrl_phys);
    pmu_base = periph_paddr_to_vaddr(driver->pmu_phys);
    ASSERT(sctrl_base && pmu_base);

    pdev_register_power(&hisi_power_ops);
}

LK_PDEV_INIT(hisi_power_init, KDRV_HISILICON_POWER, hisi_power_init, LK_INIT_LEVEL_PLATFORM);

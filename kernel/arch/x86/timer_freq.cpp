
#include <arch/x86/feature.h>
#include <arch/x86/timer_freq.h>

uint64_t x86_lookup_core_crystal_freq() {
    return x86_get_microarch_config()->get_apic_freq();
}

uint64_t x86_lookup_tsc_freq() {
    return x86_get_microarch_config()->get_tsc_freq();
}


#include <arch/arm64/hypervisor/el2_state.h>
#include <arch/arm64/hypervisor/gic/el2.h>
#include <arch/arm64/hypervisor/gic/gicv3.h>
#include <arch/ops.h>
#include <dev/interrupt/arm_gic_hw_interface.h>
#include <dev/interrupt/arm_gicv3_regs.h>
#include <vm/physmap.h>

static constexpr uint32_t kNumAprs = 4;
static constexpr uint32_t kNumLrs = 16;

static zx_status_t gicv3_get_gicv(paddr_t* gicv_paddr) {
    // Check for presence of GICv3 virtualisation extensions.
    // We return ZX_ERR_NOT_FOUND since this API is used to get
    // address of GICV base to map it to guest
    // On GICv3 we do not need to map this region, since we use system registers
    return ZX_ERR_NOT_FOUND;
}

static uint32_t gicv3_default_gich_vmcr() {
    // From ARM GIC v3/v4, Section 8.4.8: VFIQEn - In implementations where the
    // Non-secure copy of ICC_SRE_EL1.SRE is always 1, this bit is RES 1.
    return ICH_VMCR_VPMR | ICH_VMCR_VFIQEN | ICH_VMCR_VENG1;
}

static void giv3_read_gich_state(IchState* state) {
    DEBUG_ASSERT(state->num_aprs <= kNumAprs);
    DEBUG_ASSERT(state->num_lrs <= kNumLrs);
    arm64_el2_gicv3_read_gich_state(physmap_to_paddr(state));
}

static void giv3_write_gich_state(IchState* state, uint32_t hcr) {
    DEBUG_ASSERT(state->num_aprs <= kNumAprs);
    DEBUG_ASSERT(state->num_lrs <= kNumLrs);
    cpu_num_t cpu_num = arch_curr_cpu_num();
    for (uint8_t i = 0; i < state->num_lrs; i++) {
        uint64_t lr = state->lr[i];
        if (lr & ICH_LR_HARDWARE) {
            // We are adding a physical interrupt to a list register, therefore we
            // mark the physical interrupt as active on the physical distributor so
            // that the guest can deactivate it directly.
            uint32_t vector = ICH_LR_VIRTUAL_ID(lr);
            uint32_t reg = vector / 32;
            uint32_t mask = 1u << (vector % 32);
            // Since we use affinity routing, if this vector is associated with an
            // SGI or PPI, we should talk to the redistributor for the current CPU.
            if (vector < 32) {
                GICREG(0, GICR_ISACTIVER0(cpu_num)) = mask;
            } else {
                GICREG(0, GICD_ISACTIVER(reg)) = mask;
            }
        }
    }
    arm64_el2_gicv3_write_gich_state(physmap_to_paddr(state), hcr);
}

static uint64_t gicv3_get_lr_from_vector(bool hw, uint8_t prio, uint32_t vector) {
    uint64_t lr = ICH_LR_PENDING | ICH_LR_GROUP1 | ICH_LR_PRIORITY(prio) |
        ICH_LR_VIRTUAL_ID(vector);
    if (hw) {
        lr |= ICH_LR_HARDWARE | ICH_LR_PHYSICAL_ID(vector);
    }
    return lr;
}

static uint32_t gicv3_get_vector_from_lr(uint64_t lr) {
    return lr & ICH_LR_VIRTUAL_ID(UINT64_MAX);
}

static uint8_t gicv3_get_num_pres() {
    return static_cast<uint8_t>(ICH_VTR_PRES(arm64_el2_gicv3_read_gich_vtr()));
}

static uint8_t gicv3_get_num_lrs() {
    return static_cast<uint8_t>(ICH_VTR_LRS(arm64_el2_gicv3_read_gich_vtr()));
}

static const struct arm_gic_hw_interface_ops gic_hw_register_ops = {
    .get_gicv = gicv3_get_gicv,
    .read_gich_state = giv3_read_gich_state,
    .write_gich_state = giv3_write_gich_state,
    .default_gich_vmcr = gicv3_default_gich_vmcr,
    .get_lr_from_vector = gicv3_get_lr_from_vector,
    .get_vector_from_lr = gicv3_get_vector_from_lr,
    .get_num_pres = gicv3_get_num_pres,
    .get_num_lrs = gicv3_get_num_lrs,
};

void gicv3_hw_interface_register() {
    arm_gic_hw_interface_register(&gic_hw_register_ops);
}

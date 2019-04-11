
#pragma once

#include <fbl/canary.h>
#include <fbl/vector.h>
#include <kernel/mp.h>
#include <object/interrupt_dispatcher.h>
#include <sys/types.h>
#include <zircon/types.h>

class InterruptEventDispatcher final : public InterruptDispatcher {
public:
    static zx_status_t Create(fbl::RefPtr<Dispatcher>* dispatcher,
                              zx_rights_t* rights,
                              uint32_t vector,
                              uint32_t options);

    InterruptEventDispatcher(const InterruptDispatcher &) = delete;
    InterruptEventDispatcher& operator=(const InterruptDispatcher &) = delete;

    zx_status_t BindVcpu(fbl::RefPtr<VcpuDispatcher> vcpu_dispatcher) final;

private:
    explicit InterruptEventDispatcher(uint32_t vector)
        : vector_(vector) {}

    void MaskInterrupt() final;
    void UnmaskInterrupt() final;
    void UnregisterInterruptHandler() final;
    bool HasVcpu() const final;

    zx_status_t RegisterInterruptHandler();
    static interrupt_eoi IrqHandler(void* ctx);
    static interrupt_eoi VcpuIrqHandler(void* ctx);
    void VcpuInterruptHandler();

    const uint32_t vector_;
    fbl::Canary<fbl::magic("INED")> canary_;
    fbl::Vector<fbl::RefPtr<VcpuDispatcher>> vcpus_;
};

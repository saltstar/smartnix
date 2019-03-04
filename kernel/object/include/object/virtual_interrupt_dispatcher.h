
#pragma once

#include <zircon/types.h>
#include <fbl/ref_ptr.h>
#include <object/interrupt_dispatcher.h>
#include <sys/types.h>

class VirtualInterruptDispatcher final : public InterruptDispatcher {
public:
    static zx_status_t Create(fbl::RefPtr<Dispatcher>* dispatcher,
                              zx_rights_t* rights,
                              uint32_t options);

    VirtualInterruptDispatcher(const InterruptDispatcher &) = delete;
    VirtualInterruptDispatcher& operator=(const InterruptDispatcher &) = delete;

protected:
    void MaskInterrupt() final;
    void UnmaskInterrupt() final;
    void UnregisterInterruptHandler() final;

private:
    VirtualInterruptDispatcher() = default;
};

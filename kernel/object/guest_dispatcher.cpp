
#include <object/guest_dispatcher.h>

#include <arch/hypervisor.h>
#include <fbl/alloc_checker.h>
#include <object/vm_address_region_dispatcher.h>
#include <zircon/rights.h>

// static
zx_status_t GuestDispatcher::Create(fbl::RefPtr<Dispatcher>* guest_dispatcher,
                                    zx_rights_t* guest_rights,
                                    fbl::RefPtr<Dispatcher>* vmar_dispatcher,
                                    zx_rights_t* vmar_rights) {
    ktl::unique_ptr<Guest> guest;
    zx_status_t status = Guest::Create(&guest);
    if (status != ZX_OK) {
        return status;
    }

    fbl::AllocChecker ac;
    auto disp = fbl::AdoptRef(new (&ac) GuestDispatcher(ktl::move(guest)));
    if (!ac.check()) {
        return ZX_ERR_NO_MEMORY;
    }

    status = VmAddressRegionDispatcher::Create(disp->guest()->AddressSpace()->RootVmar(), 0,
                                               vmar_dispatcher, vmar_rights);
    if (status != ZX_OK) {
        return status;
    }

    *guest_rights = default_rights();
    *guest_dispatcher = ktl::move(disp);
    return ZX_OK;
}

GuestDispatcher::GuestDispatcher(ktl::unique_ptr<Guest> guest)
    : guest_(ktl::move(guest)) {}

GuestDispatcher::~GuestDispatcher() {}

zx_status_t GuestDispatcher::SetTrap(uint32_t kind, zx_vaddr_t addr, size_t len,
                                     fbl::RefPtr<PortDispatcher> port, uint64_t key) {
    canary_.Assert();
    return guest_->SetTrap(kind, addr, len, ktl::move(port), key);
}

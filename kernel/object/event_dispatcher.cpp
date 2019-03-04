
#include <object/event_dispatcher.h>

#include <err.h>

#include <zircon/rights.h>
#include <fbl/alloc_checker.h>

zx_status_t EventDispatcher::Create(uint32_t options, fbl::RefPtr<Dispatcher>* dispatcher,
                                    zx_rights_t* rights) {
    fbl::AllocChecker ac;
    auto disp = new (&ac) EventDispatcher(options);
    if (!ac.check())
        return ZX_ERR_NO_MEMORY;

    *rights = default_rights();
    *dispatcher = fbl::AdoptRef<Dispatcher>(disp);
    return ZX_OK;
}

EventDispatcher::EventDispatcher(uint32_t options) {}

EventDispatcher::~EventDispatcher() {}

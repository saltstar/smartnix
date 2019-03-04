
#pragma once

#include <zircon/rights.h>
#include <zircon/types.h>

#include <fbl/canary.h>
#include <object/dispatcher.h>

#include <sys/types.h>

class EventDispatcher final :
    public SoloDispatcher<EventDispatcher, ZX_DEFAULT_EVENT_RIGHTS, ZX_EVENT_SIGNALED> {
public:
    static zx_status_t Create(uint32_t options, fbl::RefPtr<Dispatcher>* dispatcher,
                              zx_rights_t* rights);

    ~EventDispatcher() final;
    zx_obj_type_t get_type() const final { return ZX_OBJ_TYPE_EVENT; }
    CookieJar* get_cookie_jar() final { return &cookie_jar_; }

private:
    explicit EventDispatcher(uint32_t options);
    fbl::Canary<fbl::magic("EVTD")> canary_;
    CookieJar cookie_jar_;
};

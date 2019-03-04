
#pragma once

#include <zircon/rights.h>
#include <zircon/types.h>

#include <zircon/syscalls/profile.h>

#include <fbl/canary.h>
#include <object/dispatcher.h>

class ProfileDispatcher final :
    public SoloDispatcher<ProfileDispatcher, ZX_DEFAULT_PROFILE_RIGHTS> {
public:
    static zx_status_t Create(const zx_profile_info_t& info,
                              fbl::RefPtr<Dispatcher>* dispatcher,
                              zx_rights_t* rights);

    ~ProfileDispatcher() final;
    zx_obj_type_t get_type() const final { return ZX_OBJ_TYPE_PROFILE; }

    zx_status_t ApplyProfile(fbl::RefPtr<ThreadDispatcher> thread);

private:
    explicit ProfileDispatcher(const zx_profile_info_t& info);

    fbl::Canary<fbl::magic("PROF")> canary_;
    const zx_profile_info_t info_;
};


#include <err.h>
#include <inttypes.h>

#include <lib/counters.h>
#include <lib/ktrace.h>

#include <object/handle.h>
#include <object/job_dispatcher.h>
#include <object/profile_dispatcher.h>

#include <zircon/types.h>

#include <fbl/ref_ptr.h>

#include "priv.h"

KCOUNTER(profile_create, "kernel.profile.create");
KCOUNTER(profile_set,    "kernel.profile.set");


// zx_status_t zx_profile_create
zx_status_t sys_profile_create(zx_handle_t root_job,
                               user_in_ptr<const zx_profile_info_t> user_profile_info,
                               user_out_handle* out) {
    // TODO(ZX-3352): check job policy.

    auto up = ProcessDispatcher::GetCurrent();

    fbl::RefPtr<JobDispatcher> job;
    auto status = up->GetDispatcherWithRights(root_job, ZX_RIGHT_MANAGE_PROCESS, &job);
    if (status != ZX_OK) {
        return status;
    }

    // Validate that the job is in fact the first usermode job (aka root job).
    if (GetRootJobDispatcher() != job->parent()) {
        // TODO(cpu): consider a better error code.
        return ZX_ERR_ACCESS_DENIED;
    }

    zx_profile_info_t profile_info;
    status = user_profile_info.copy_from_user(&profile_info);
    if (status != ZX_OK) {
        return status;
    }

    fbl::RefPtr<Dispatcher> dispatcher;
    zx_rights_t rights;
    status = ProfileDispatcher::Create(profile_info, &dispatcher, &rights);
    if (status != ZX_OK) {
        return status;
    }

    kcounter_add(profile_create, 1);

    return out->make(ktl::move(dispatcher), rights);
}

// zx_status_t zx_object_set_profile
zx_status_t sys_object_set_profile(zx_handle_t handle,
                                   zx_handle_t profile_handle,
                                   uint32_t options) {
    auto up = ProcessDispatcher::GetCurrent();

    // TODO(cpu): support more than thread objects, and actually do something.

    fbl::RefPtr<ThreadDispatcher> thread;
    auto status = up->GetDispatcherWithRights(handle, ZX_RIGHT_MANAGE_THREAD, &thread);
    if (status != ZX_OK)
        return status;

    fbl::RefPtr<ProfileDispatcher> profile;
    zx_status_t result =
        up->GetDispatcherWithRights(profile_handle, ZX_RIGHT_APPLY_PROFILE, &profile);
    if (result != ZX_OK)
        return result;

    kcounter_add(profile_set, 1);

    return profile->ApplyProfile(ktl::move(thread));
}

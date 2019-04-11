
#include <err.h>
#include <platform.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <trace.h>

#include <object/excp_port.h>
#include <object/job_dispatcher.h>
#include <object/port_dispatcher.h>
#include <object/process_dispatcher.h>
#include <object/thread_dispatcher.h>

#include "priv.h"

#define LOCAL_TRACE 0

static zx_status_t object_unbind_exception_port(zx_handle_t obj_handle, bool debugger) {
    // TODO(ZX-968): check rights once appropriate right is determined
    auto up = ProcessDispatcher::GetCurrent();

    fbl::RefPtr<Dispatcher> dispatcher;
    auto status = up->GetDispatcher(obj_handle, &dispatcher);
    if (status != ZX_OK)
        return status;

    auto job = DownCastDispatcher<JobDispatcher>(&dispatcher);
    if (job) {
        return job->ResetExceptionPort(debugger) ? ZX_OK : ZX_ERR_BAD_STATE; // No port was bound.
    }

    auto process = DownCastDispatcher<ProcessDispatcher>(&dispatcher);
    if (process) {
        return process->ResetExceptionPort(debugger) ? ZX_OK
                                                     : ZX_ERR_BAD_STATE; // No port was bound.
    }

    auto thread = DownCastDispatcher<ThreadDispatcher>(&dispatcher);
    if (thread) {
        if (debugger)
            return ZX_ERR_INVALID_ARGS;
        return thread->ResetExceptionPort() ? ZX_OK : ZX_ERR_BAD_STATE; // No port was bound.
    }

    return ZX_ERR_WRONG_TYPE;
}

static zx_status_t task_bind_exception_port(zx_handle_t obj_handle, zx_handle_t eport_handle, uint64_t key, bool debugger) {
    // TODO(ZX-968): check rights once appropriate right is determined
    auto up = ProcessDispatcher::GetCurrent();

    fbl::RefPtr<PortDispatcher> port;
    zx_status_t status = up->GetDispatcher(eport_handle, &port);
    if (status != ZX_OK)
        return status;

    fbl::RefPtr<Dispatcher> dispatcher;
    status = up->GetDispatcher(obj_handle, &dispatcher);
    if (status != ZX_OK)
        return status;

    fbl::RefPtr<ExceptionPort> eport;

    auto job = DownCastDispatcher<JobDispatcher>(&dispatcher);
    if (job) {
        ExceptionPort::Type type;
        if (debugger)
            type = ExceptionPort::Type::JOB_DEBUGGER;
        else
            type = ExceptionPort::Type::JOB;
        status = ExceptionPort::Create(type,
                                       ktl::move(port), key, &eport);
        if (status != ZX_OK)
            return status;
        status = job->SetExceptionPort(eport);
        if (status != ZX_OK)
            return status;

        eport->SetTarget(job);
        return ZX_OK;
    }

    auto process = DownCastDispatcher<ProcessDispatcher>(&dispatcher);
    if (process) {
        ExceptionPort::Type type;
        if (debugger)
            type = ExceptionPort::Type::DEBUGGER;
        else
            type = ExceptionPort::Type::PROCESS;
        status = ExceptionPort::Create(type, ktl::move(port), key, &eport);
        if (status != ZX_OK)
            return status;
        status = process->SetExceptionPort(eport);
        if (status != ZX_OK)
            return status;

        eport->SetTarget(process);
        return ZX_OK;
    }

    auto thread = DownCastDispatcher<ThreadDispatcher>(&dispatcher);
    if (thread) {
        if (debugger)
            return ZX_ERR_INVALID_ARGS;
        status = ExceptionPort::Create(ExceptionPort::Type::THREAD,
                                       ktl::move(port), key, &eport);
        if (status != ZX_OK)
            return status;
        status = thread->SetExceptionPort(eport);
        if (status != ZX_OK)
            return status;

        eport->SetTarget(thread);
        return ZX_OK;
    }

    return ZX_ERR_WRONG_TYPE;
}

// zx_status_t zx_task_bind_exception_port
zx_status_t sys_task_bind_exception_port(zx_handle_t handle, zx_handle_t port,
                                           uint64_t key, uint32_t options) {
    LTRACE_ENTRY;

    if (options & ~ZX_EXCEPTION_PORT_DEBUGGER)
        return ZX_ERR_INVALID_ARGS;

    bool debugger = (options & ZX_EXCEPTION_PORT_DEBUGGER) != 0;

    if (port == ZX_HANDLE_INVALID) {
        return object_unbind_exception_port(handle, debugger);
    } else {
        return task_bind_exception_port(handle, port, key, debugger);
    }
}

// zx_status_t zx_task_resume_from_exception
zx_status_t sys_task_resume_from_exception(zx_handle_t handle, zx_handle_t port, uint32_t options) {
    LTRACE_ENTRY;

    auto up = ProcessDispatcher::GetCurrent();

    fbl::RefPtr<ThreadDispatcher> thread;
    zx_status_t status = up->GetDispatcher(handle, &thread);
    if (status != ZX_OK)
        return status;

    fbl::RefPtr<PortDispatcher> eport;
    status = up->GetDispatcher(port, &eport);
    if (status != ZX_OK)
        return status;

    // Currently the only option is the ZX_RESUME_TRY_NEXT bit.
    if (options != 0 && options != ZX_RESUME_TRY_NEXT)
        return ZX_ERR_INVALID_ARGS;

    if (options & ZX_RESUME_TRY_NEXT) {
        return thread->MarkExceptionNotHandled(eport.get());
    } else {
        return thread->MarkExceptionHandled(eport.get());
    }
}

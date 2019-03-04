
#pragma once

// clang-format off

#include <stdint.h>
#include <zircon/device/ioctl.h>
#include <zircon/device/ioctl-wrapper.h>

__BEGIN_CDECLS

// Get a channel handle for a two-way QMI channel for sending and
// receiving QMI requests/responses/indications
//   in: none
//   out: handle to channel
#define IOCTL_QMI_GET_CHANNEL \
    IOCTL(IOCTL_KIND_GET_HANDLE, IOCTL_FAMILY_QMI, 0)

IOCTL_WRAPPER_OUT(ioctl_qmi_get_channel, IOCTL_QMI_GET_CHANNEL, zx_handle_t);

__END_CDECLS

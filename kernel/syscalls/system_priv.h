
#pragma once

#include <zircon/syscalls/system.h>

zx_status_t arch_system_powerctl(uint32_t cmd, const zx_system_powerctl_arg_t* arg);

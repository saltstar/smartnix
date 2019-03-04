
#pragma once

#include <zircon/syscalls/smc.h>

zx_status_t arch_smc_call(const zx_smc_parameters_t* params, zx_smc_result_t* result);

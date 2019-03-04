
#pragma once

#include <lib/smbios/smbios.h>

void pc_init_smbios();

// Walk the known SMBIOS structures.  The callback will be called once for each
// structure found.
zx_status_t SmbiosWalkStructs(smbios::StructWalkCallback cb, void* ctx);

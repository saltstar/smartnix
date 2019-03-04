
#pragma once

#include <lib/svc/service.h>
#include <zircon/compiler.h>

__BEGIN_CDECLS

const zx_service_provider_t* logger_get_service_provider(void);

__END_CDECLS


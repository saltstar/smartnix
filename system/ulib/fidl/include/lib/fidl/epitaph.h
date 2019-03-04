
#ifndef LIB_FIDL_EPITAPH_H_
#define LIB_FIDL_EPITAPH_H_

#include <zircon/types.h>

__BEGIN_CDECLS

// Sends an epitaph with the given values down the channel.
// See https://fuchsia.googlesource.com/docs/+/master/development/languages/fidl/languages/c.md#fidl_epitaph_write
zx_status_t fidl_epitaph_write(zx_handle_t channel, zx_status_t error);

__END_CDECLS

#endif // LIB_FIDL_EPITAPH_H_

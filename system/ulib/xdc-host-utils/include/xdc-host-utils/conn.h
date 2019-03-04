
#pragma once

#include <stdint.h>

namespace xdc {

// The UNIX domain socket path for the host xdc server.
static const char* XDC_SOCKET_PATH = "/tmp/xdc";

// For clients registering a host stream id.
using RegisterStreamRequest  = uint32_t;
using RegisterStreamResponse = bool;

}  // namespace xdc


#pragma once

#include <stdint.h>
#include <stdio.h>

namespace devmgr {

#define LOG_ERROR    0x001
#define LOG_INFO     0x002
#define LOG_TRACE    0x004
#define LOG_SPEW     0x008
#define LOG_RPC_IN   0x010
#define LOG_RPC_OUT  0x020
#define LOG_RPC_RIO  0x040
#define LOG_RPC_SDW  0x080
#define LOG_DEVFS    0x100
#define LOG_DEVLC    0x200
#define LOG_ALL      0x3ff

extern uint32_t log_flags;

#define log(flag, fmt...) do { if (LOG_##flag & log_flags) printf(fmt); } while (0)

} // namespace devmgr

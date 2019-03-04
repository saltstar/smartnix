
#pragma once

void initialize_status(const char* name, size_t size);
void update_status(size_t bytes_so_far);
int tftp_xfer(struct sockaddr_in6* addr, const char* fn, const char* name);
int netboot_xfer(struct sockaddr_in6* addr, const char* fn, const char* name);

#define DEFAULT_TFTP_BLOCK_SZ 1428
#define DEFAULT_TFTP_WIN_SZ 1024
#define DEFAULT_US_BETWEEN_PACKETS 20

extern char* appname;
extern int64_t us_between_packets;
extern uint16_t* tftp_block_size;
extern uint16_t* tftp_window_size;

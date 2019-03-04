
#pragma once

#include <block-client/cpp/client.h>
#include <fbl/unique_fd.h>

// Ensures a block client has synchronized all operations to storage.
zx_status_t FlushClient(const block_client::Client& client);

// Ensures a block device has synchronized all operations to storage.
zx_status_t FlushBlockDevice(const fbl::unique_fd& fd);


#pragma once

#include <stdint.h>
#include <kernel/thread.h>
#include <zircon/types.h>

// You probably don't want to use this class.
class Semaphore {
public:
    explicit Semaphore(int64_t initial_count = 0);
    ~Semaphore();

    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    void Post();

    zx_status_t Wait(const Deadline& deadline);

private:
    int64_t count_;
    WaitQueue waitq_;
};


#pragma once

#include <kernel/spinlock.h>
#include <ktl/unique_ptr.h>
#include <sys/types.h>

#include <bitmap/rle-bitmap.h>

class IoBitmap {
public:
    // Return the IoBitmap associated with the current thread.
    static IoBitmap& GetCurrent();

    ~IoBitmap();

    int SetIoBitmap(uint32_t port, uint32_t len, bool enable);

private:
    // Task used for updating IO permissions on each CPU.
    static void UpdateTask(void* context);

    friend void x86_set_tss_io_bitmap(IoBitmap& bitmap);
    friend void x86_clear_tss_io_bitmap(IoBitmap& bitmap);

    ktl::unique_ptr<bitmap::RleBitmap> bitmap_;
    SpinLock lock_;
};

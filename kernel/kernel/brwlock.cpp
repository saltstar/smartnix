
#include <kernel/brwlock.h>
#include <kernel/thread_lock.h>

BrwLock::~BrwLock() {
    DEBUG_ASSERT(state_.load(fbl::memory_order_relaxed) == 0);
}

void BrwLock::Block(bool write) {
    thread_t* ct = get_current_thread();

    if (state_.load(fbl::memory_order_relaxed) & kBrwLockWriter) {
        thread_t* writer_copy = writer_.load(fbl::memory_order_relaxed);
        // Boost the writers priority. As we have already registered ourselves as
        // a waiter and we currently hold the thread_lock there is no race with
        // a writer performing a release as it will be forced to acquire the
        // thread_lock prior to deboosting itself.
        // The check against nullptr here is to resolve the unlikely race in
        // CommonWriteAcquire.
        if (writer_copy != nullptr) {
            bool unused;
            sched_inherit_priority(writer_copy, ct->effec_priority, &unused);
        }
    }

    zx_status_t ret =
        write ? wait_.Block(Deadline::infinite()) : wait_.BlockReadLock(Deadline::infinite());
    if (unlikely(ret < ZX_OK)) {
        panic("BrwLock::Block: wait_queue_block returns with error %d lock %p, thr %p, sp %p\n",
              ret, this, ct, __GET_FRAME());
    }
}

void BrwLock::WakeThread(thread_t* thread, uint64_t adjust) {
    state_.fetch_add(-kBrwLockWaiter + adjust, fbl::memory_order_acq_rel);
    zx_status_t status = wait_.UnblockThread(thread, ZX_OK);
    if (status != ZX_OK) {
        panic("Tried to unblock thread from wait queue that was not blocked");
    }
}

void BrwLock::WakeReaders() {
    while (!wait_.IsEmpty()) {
        thread_t* next = wait_.Peek();
        if (next->state != THREAD_BLOCKED_READ_LOCK) {
            break;
        }
        WakeThread(next, kBrwLockReader);
    }
}

void BrwLock::WakeWriter(thread_t* thread) {
    DEBUG_ASSERT(thread);
    writer_.store(thread, fbl::memory_order_relaxed);
    WakeThread(thread, kBrwLockWriter);
}

void BrwLock::WakeAny() {
    thread_t* next = wait_.Peek();
    DEBUG_ASSERT(next != NULL);
    if (next->state == THREAD_BLOCKED_READ_LOCK) {
        WakeReaders();
    } else {
        WakeWriter(next);
    }
}

void BrwLock::ContendedReadAcquire() {
    Guard<spin_lock_t, IrqSave> guard{ThreadLock::Get()};
    // Remove our optimistic reader from the count, and put a waiter on there instead.
    uint64_t prev = state_.fetch_add(-kBrwLockReader + kBrwLockWaiter, fbl::memory_order_relaxed);
    // If there is a writer then we just block, they will wake us up
    if (prev & kBrwLockWriter) {
        Block(false);
        return;
    }
    // If we raced and there is in fact no one waiting then we can switch to
    // having the lock
    if ((prev & kBrwLockWaiterMask) == 0) {
        state_.fetch_add(-kBrwLockWaiter + kBrwLockReader, fbl::memory_order_acquire);
        return;
    }
    thread_t* next = wait_.Peek();
    DEBUG_ASSERT(next != NULL);
    if (next->state == THREAD_BLOCKED_READ_LOCK) {
        WakeReaders();
        // Join the reader pool.
        state_.fetch_add(-kBrwLockWaiter + kBrwLockReader, fbl::memory_order_acquire);
        return;
    }
    // If there are no current readers then we unblock this writer, since
    // otherwise nobody will be using the lock.
    if ((prev & kBrwLockReaderMask) == 1) {
        WakeWriter(next);
    }

    Block(false);
}

void BrwLock::ContendedWriteAcquire() {
    Guard<spin_lock_t, IrqSave> guard{ThreadLock::Get()};
    // Mark ourselves as waiting
    uint64_t prev = state_.fetch_add(kBrwLockWaiter, fbl::memory_order_relaxed);
    // If there is a writer then we just block, they will wake us up
    if (prev & kBrwLockWriter) {
        Block(true);
        return;
    }
    if ((prev & kBrwLockReaderMask) == 0) {
        if ((prev & kBrwLockWaiterMask) == 0) {
            writer_.store(get_current_thread(), fbl::memory_order_relaxed);
            // Must have raced previously as turns out there's no readers or
            // waiters, so we can convert to having the lock
            state_.fetch_add(-kBrwLockWaiter + kBrwLockWriter, fbl::memory_order_acquire);
            return;
        } else {
            // There's no readers, but someone already waiting, wake up someone
            // before we ourselves block
            WakeAny();
        }
    }
    Block(true);
}

void BrwLock::WriteRelease() TA_NO_THREAD_SAFETY_ANALYSIS {
    canary_.Assert();
    thread_t* ct = get_current_thread();

#if LK_DEBUG_LEVEL > 0
    thread_t* holder = writer_.load(fbl::memory_order_relaxed);
    if (unlikely(ct != holder)) {
        panic("BrwLock::WriteRelease: thread %p (%s) tried to release brwlock %p it doesn't "
              "own. Ownedby %p (%s)\n",
              ct, ct->name, this, holder, holder ? holder->name : "none");
    }
#endif

    // Drop the `writer` before updating `state`. The race here of another thread
    // observing a null `writer` and 'failing' to do PI in `Block` does not matter
    // since we're already doing release and would only immediately give the
    // donation back.
    writer_.store(nullptr, fbl::memory_order_relaxed);
    uint64_t prev = state_.fetch_sub(kBrwLockWriter, fbl::memory_order_release);
    ct->mutexes_held--;

    // Perform release wakeup prior to deboosting our priority as we can be
    // certain we aren't racing with someone trying to Block after that
    if (unlikely((prev & kBrwLockWaiterMask) != 0)) {
        // There are waiters, we need to wake them up
        ReleaseWakeup();
    }

    // deboost ourselves if this is the last mutex we held.
    if (ct->inherited_priority >= 0 && ct->mutexes_held == 0) {
        bool local_resched = false;
        Guard<spin_lock_t, IrqSave> guard{ThreadLock::Get()};
        sched_inherit_priority(ct, -1, &local_resched);
        if (local_resched) {
            sched_reschedule();
        }
    }
}

void BrwLock::ReleaseWakeup() {
    Guard<spin_lock_t, IrqSave> guard{ThreadLock::Get()};
    uint64_t count = state_.load(fbl::memory_order_relaxed);
    if ((count & kBrwLockWaiterMask) != 0 && (count & kBrwLockWriter) == 0 &&
        (count & kBrwLockReaderMask) == 0) {
        WakeAny();
    }
}

void BrwLock::ContendedReadUpgrade() {
    Guard<spin_lock_t, IrqSave> guard{ThreadLock::Get()};

    // Convert our reading into waiting
    uint64_t prev = state_.fetch_add(-kBrwLockReader + kBrwLockWaiter, fbl::memory_order_relaxed);
    if ((prev & ~kBrwLockWaiterMask) == kBrwLockReader) {
        writer_.store(get_current_thread(), fbl::memory_order_relaxed);
        // There are no writers or readers. There might be waiters, but as we
        // already have some form of lock we still have fairness even if we
        // bypass the queue, so we convert our waiting into writing
        state_.fetch_add(-kBrwLockWaiter + kBrwLockWriter, fbl::memory_order_acquire);
    } else {
        Block(true);
    }
}

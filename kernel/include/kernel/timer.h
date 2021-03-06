
#pragma once

#include <kernel/deadline.h>
#include <kernel/spinlock.h>
#include <list.h>
#include <sys/types.h>
#include <zircon/compiler.h>
#include <zircon/types.h>

__BEGIN_CDECLS

void timer_queue_init(void);

struct timer;
typedef void (*timer_callback)(struct timer*, zx_time_t now, void* arg);

#define TIMER_MAGIC (0x74696D72) //'timr'

typedef struct timer {
    int magic;
    struct list_node node;

    zx_time_t scheduled_time;
    zx_duration_t slack; // Stores the applied slack adjustment from
    //                      the ideal scheduled_time.
    timer_callback callback;
    void* arg;

    volatile int active_cpu; // <0 if inactive
    volatile bool cancel;    // true if cancel is pending
} timer_t;

#define TIMER_INITIAL_VALUE(t)              \
    {                                       \
        .magic = TIMER_MAGIC,               \
        .node = LIST_INITIAL_CLEARED_VALUE, \
        .scheduled_time = 0,                \
        .slack = 0,                         \
        .callback = NULL,                   \
        .arg = NULL,                        \
        .active_cpu = -1,                   \
        .cancel = false,                    \
    }

// Rules for Timers:
// - Timer callbacks occur from interrupt context
// - Timers may be programmed or canceled from interrupt or thread context
// - Timers may be canceled or reprogrammed from within their callback
// - Setting and canceling timers is not thread safe and cannot be done concurrently
// - timer_cancel() may spin waiting for a pending timer to complete on another cpu

// Initialize a timer object
void timer_init(timer_t*);

//
// Set up a timer that executes once
//
// This function specifies a callback function to be run after a specified
// deadline passes. The function will be called one time.
//
// timer: the timer to use
// deadline: specifies when the timer should be executed
// callback: the function to call when the timer expires
// arg: the argument to pass to the callback
//
// The timer function is declared as:
//   void callback(timer_t *, zx_time_t now, void *arg) { ... }
void timer_set(timer_t* timer, const Deadline& deadline, timer_callback callback, void* arg);

//
// Cancel a pending timer
//
// Returns true if the timer was canceled before it was
// scheduled in a cpu and false otherwise or if the timer
// was not scheduled at all.
//
bool timer_cancel(timer_t*);

// Equivalent to timer_set with no slack
static inline void timer_set_oneshot(
    timer_t* timer, zx_time_t deadline, timer_callback callback, void* arg) {
    return timer_set(timer, Deadline::no_slack(deadline), callback, arg);
}

// Preemption Timers
//
// Each CPU has a dedicated preemption timer that's managed using specialized functions (prefixed
// with timer_preempt_).
//
// Preemption timers are different from general timers. Preemption timers:
//
// - are reset frequently by the scheduler so performance is important
// - should not be migrated off their CPU when the CPU is shutdown
//
// Note: A preemption timer may fire even after it has been canceled.
//

//
// Set/reset the current CPU's preemption timer.
//
// When the preemption timer fires, sched_preempt_timer_tick is called.
void timer_preempt_reset(zx_time_t deadline);

//
// Cancel the current CPU's preemption timer.
void timer_preempt_cancel(void);

// Internal routines used when bringing cpus online/offline

// Moves |old_cpu|'s timers (except its preemption timer) to the current cpu
void timer_transition_off_cpu(uint old_cpu);

// This function is to be invoked after resume on each CPU that may have
// had timers still on it, in order to restart hardware timers.
void timer_thaw_percpu(void);

// Special helper routine to simultaneously try to acquire a spinlock and check for
// timer cancel, which is needed in a few special cases.
// returns ZX_OK if spinlock was acquired, ZX_ERR_TIMED_OUT if timer was canceled.
zx_status_t timer_trylock_or_cancel(timer_t* t, spin_lock_t* lock) TA_TRY_ACQ(false, lock);

__END_CDECLS


#pragma once

__BEGIN_CDECLS

/* use the cpu local thread context pointer to store current_thread */
static inline struct thread* get_current_thread(void) {
#ifdef __clang__
    // Clang with --target=aarch64-fuchsia -mcmodel=kernel reads
    // TPIDR_EL1 for __builtin_thread_pointer (instead of the usual
    // TPIDR_EL0 for user mode).  Using the intrinsic instead of asm
    // lets the compiler understand what it's doing a little better,
    // which conceivably could let it optimize better.
    char* tp = (char*)__builtin_thread_pointer();
#else
    char* tp = (char*)__arm_rsr64("tpidr_el1");
#endif
    tp -= offsetof(struct thread, arch.thread_pointer_location);
    return (struct thread*)tp;
}

static inline void set_current_thread(struct thread* t) {
    __arm_wsr64("tpidr_el1", (uint64_t)&t->arch.thread_pointer_location);
    __isb(ARM_MB_SY);
}

__END_CDECLS

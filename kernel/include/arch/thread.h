
#pragma once

// give the arch code a chance to declare the arch_thread struct
#include <arch/arch_thread.h>
#include <zircon/compiler.h>

__BEGIN_CDECLS

struct thread;

void arch_thread_initialize(struct thread *, vaddr_t entry_point);
void arch_context_switch(struct thread *oldthread, struct thread *newthread);
void arch_thread_construct_first(struct thread *);
void* arch_thread_get_blocked_fp(struct thread *);

__END_CDECLS

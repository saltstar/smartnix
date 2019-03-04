
#include <kernel/percpu.h>

#include <arch/ops.h>
#include <kernel/align.h>

struct percpu percpu[SMP_MAX_CPUS] __CPU_ALIGN_EXCLUSIVE;

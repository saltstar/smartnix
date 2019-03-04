
#include <rand.h>

#include <kernel/atomic.h>
#include <sys/types.h>

static uint64_t randseed;

void srand(unsigned int seed) {
    atomic_store_u64_relaxed(&randseed, seed - 1);
}

int rand(void) {
    for (;;) {
        uint64_t old_seed = atomic_load_u64_relaxed(&randseed);
        uint64_t new_seed = 6364136223846793005ULL * old_seed + 1;
        if (atomic_cmpxchg_u64_relaxed(&randseed, &old_seed, new_seed)) {
            return new_seed >> 33;
        }
    }
}

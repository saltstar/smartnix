
#pragma once

#include <zircon/compiler.h>
#include <sys/types.h>

__BEGIN_CDECLS

#define RAND_MAX (0x7fffffff)

int rand(void);
void srand(unsigned int seed);

__END_CDECLS

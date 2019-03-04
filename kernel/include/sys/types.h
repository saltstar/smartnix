
#pragma once

#include <limits.h>
#include <stdint.h>
#include <stddef.h>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

typedef uintptr_t addr_t;
typedef uintptr_t vaddr_t;
typedef uintptr_t paddr_t;

typedef intptr_t ssize_t;
#define SSIZE_MAX INTPTR_MAX

#define KB (1024UL)
#define MB (1024UL * KB)
#define GB (1024UL * MB)

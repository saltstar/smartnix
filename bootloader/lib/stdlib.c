
#include <stdlib.h>

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>

#define ATOx(T, fn)             \
T fn(const char* nptr) {            \
    while (nptr && isspace(*nptr)) {  \
        nptr++;                       \
    }                                 \
                                      \
    bool neg = false;                 \
    if (*nptr == '-') {               \
        neg = true;                   \
        nptr++;                       \
    }                                 \
                                      \
    T ret = 0;                        \
    for (; nptr; nptr++) {            \
        if (!isdigit(*nptr)) break;   \
        ret = 10*ret + (*nptr - '0'); \
    }                                 \
                                      \
    if (neg) ret = -ret;              \
    return ret;                       \
}


ATOx(int, atoi)
ATOx(long, atol)
ATOx(long long, atoll)

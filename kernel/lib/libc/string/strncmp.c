
#include <string.h>
#include <sys/types.h>

int
strncmp(char const *cs, char const *ct, size_t count) {
    signed char __res = 0;

    while (count > 0) {
        if ((__res = *cs - *ct++) != 0 || !*cs++)
            break;
        count--;
    }

    return __res;
}


#include <string.h>
#include <sys/types.h>

int
strcmp(char const *cs, char const *ct) {
    signed char __res;

    while (1) {
        if ((__res = *cs - *ct++) != 0 || !*cs++)
            break;
    }

    return __res;
}

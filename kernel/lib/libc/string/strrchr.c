
#include <string.h>
#include <sys/types.h>

char *
strrchr(char const *s, int c) {
    char const *last= c?0:s;


    while (*s) {
        if (*s== c) {
            last= s;
        }

        s+= 1;
    }

    return (char *)last;
}

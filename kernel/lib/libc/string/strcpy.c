
#include <string.h>
#include <sys/types.h>

char *
strcpy(char *dest, char const *src) {
    char *tmp = dest;

    while ((*dest++ = *src++) != '\0')
        ;
    return tmp;
}


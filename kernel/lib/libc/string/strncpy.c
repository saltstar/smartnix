
#include <string.h>
#include <sys/types.h>

char *
strncpy(char *dest, char const *src, size_t count) {
    char *tmp = dest;

    while (count-- && (*dest++ = *src++) != '\0')
        ;

    return tmp;
}


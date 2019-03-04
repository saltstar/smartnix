
#include <string.h>
#include <sys/types.h>

char *
strcat(char *dest,  char const *src) {
    char *tmp = dest;

    while (*dest)
        dest++;
    while ((*dest++ = *src++) != '\0')
        ;

    return tmp;
}


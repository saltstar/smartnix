
#include <string.h>
#include <sys/types.h>

size_t
strnlen(char const *s, size_t count) {
    const char *sc;

    for (sc = s; count-- && *sc != '\0'; ++sc)
        ;
    return sc - s;
}

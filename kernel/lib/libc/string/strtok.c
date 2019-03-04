
#include <string.h>
#include <sys/types.h>

static char *___strtok = NULL;

char *
strtok(char *s, char const *ct) {
    char *sbegin, *send;

    sbegin  = s ? s : ___strtok;
    if (!sbegin) {
        return NULL;
    }
    sbegin += strspn(sbegin,ct);
    if (*sbegin == '\0') {
        ___strtok = NULL;
        return ( NULL );
    }
    send = strpbrk( sbegin, ct);
    if (send && *send != '\0')
        *send++ = '\0';
    ___strtok = send;
    return (sbegin);
}


#include <string.h>
#include <sys/types.h>

void *
memchr(void const *buf, int c, size_t len) {
    size_t i;
    unsigned char const *b= buf;
    unsigned char        x= (c&0xff);

    for (i= 0; i< len; i++) {
        if (b[i]== x) {
            return (void *)(b+i);
        }
    }

    return NULL;
}


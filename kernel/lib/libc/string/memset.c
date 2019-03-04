
#include <string.h>
#include <sys/types.h>

void *
memset(void *s, int c, size_t count) {
    char *xs = (char *) s;
    size_t len = (-(size_t)s) & (sizeof(size_t)-1);
    size_t cc = c & 0xff;

    if ( count > len ) {
        count -= len;
        cc |= cc << 8;
        cc |= cc << 16;
        if (sizeof(size_t) == 8)
            cc |= (uint64_t)cc << 32; // should be optimized out on 32 bit machines

        // write to non-aligned memory byte-wise
        for ( ; len > 0; len-- )
            *xs++ = c;

        // write to aligned memory dword-wise
        for ( len = count/sizeof(size_t); len > 0; len-- ) {
            *((size_t *)xs) = (size_t)cc;
            xs += sizeof(size_t);
        }

        count &= sizeof(size_t)-1;
    }

    // write remaining bytes
    for ( ; count > 0; count-- )
        *xs++ = c;

    return s;
}

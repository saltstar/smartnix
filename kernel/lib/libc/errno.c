
#include <errno.h>

/* completely un-threadsafe implementation of errno */
/* TODO: pull from kernel TLS or some other thread local storage */
static int _errno;

int *__geterrno(void) {
    return &_errno;
}


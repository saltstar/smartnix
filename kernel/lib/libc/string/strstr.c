
#include <string.h>
#include <sys/types.h>

char *
strstr(char const *s1, char const *s2) {
    int l1, l2;

    l2 = strlen(s2);
    if (!l2)
        return (char *)s1;
    l1 = strlen(s1);
    while (l1 >= l2) {
        l1--;
        if (!memcmp(s1,s2,l2))
            return (char *)s1;
        s1++;
    }
    return NULL;
}

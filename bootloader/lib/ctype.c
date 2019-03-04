
#include <ctype.h>

int isdigit(int c) {
    return (c >= '0') && (c <= '9');
}

int isspace(int c) {
    return (c == ' ')  ||
           (c == '\f') ||
           (c == '\n') ||
           (c == '\r') ||
           (c == '\t') ||
           (c == '\v');
}

int tolower(int c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}


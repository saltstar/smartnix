

/* nulled out atexit. static object constructors call this */
int atexit(void (*func)(void));
int atexit(void (*func)(void)) {
    return 0;
}


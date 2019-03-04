
#include <target.h>

#include <debug.h>
#include <err.h>
#include <zircon/compiler.h>

/*
 * default implementations of these routines, if the target code
 * chooses not to implement.
 */

__WEAK void target_early_init() {
}

__WEAK void target_init() {
}

__WEAK void target_quiesce() {
}

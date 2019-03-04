
#pragma once

#include <stdarg.h>
#include <zircon/compiler.h>
#include <stddef.h>

__BEGIN_CDECLS

/* printf engine that parses the format string and generates output */

/* function pointer to pass the printf engine, called back during the formatting.
 * input is a string to output, length bytes to output,
 * return code is number of characters that would have been written, or error code (if negative)
 */
typedef int (*_printf_engine_output_func)(const char *str, size_t len, void *state);

#ifndef PRINTF_DECL
#define PRINTF_DECL(name) int name
#endif
#ifndef PRINTF_CALL
#define PRINTF_CALL(name) name
#endif

PRINTF_DECL(_printf_engine)(_printf_engine_output_func out, void *state, const char *fmt, va_list ap);

__END_CDECLS

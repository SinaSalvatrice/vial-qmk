#ifndef PRINTF_H
#define PRINTF_H

#include <stdarg.h>
#include <stdio.h>

static inline int my_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);
    return ret;
}

#endif // PRINTF_H

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _ISOC99_SOURCE
#define _ISOC99_SOURCE
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
/* #include <fcntl.h> */
/* #include <unistd.h> */

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

char *
concat(const char *s1, ...);
size_t
lconcat(char *dst, size_t dstsize, const char *s1, ...);

char *
concat(const char *s1, ...)
{
    va_list args;
    const char *s;
    char *p, *result;
    size_t l, m, n;

    if (!s1) return NULL;

    m = n = strlen(s1);
    va_start(args, s1);
    while ((s = va_arg(args, char *))) {
        l = strlen(s);
        if ((m += l) < l)
            break;
    }
    va_end(args);
    if (s || m >= INT_MAX)
        return NULL;

    result = (char *)malloc(m + 1);
    if (!result)
        return NULL;

    memcpy(p = result, s1, n);
    p += n;
    va_start(args, s1);
    while ((s = va_arg(args, char *))) {
        l = strlen(s);
        if ((n += l) < l || n > m)
            break;
        memcpy(p, s, l);
        p += l;
    }
    va_end(args);
    if (s || m != n || p != result + n) {
        free(result);
        return NULL;
    }

    *p = '\0';
    return result;
}
size_t
lconcat(char *dst, size_t dstsize, const char *s1, ...)
{
    va_list args;
    size_t l, m;
    const char *s;
    if (!s1) {
        if (dst && dstsize) *dst = '\0';
        return 0;
    }
    s = s1;
    l = m = strlen(s);
    if (dst && dstsize) {
        char *p = dst;
        if (m < dstsize) {
            memcpy(p, s, l);
            p += l;
        }
        va_start(args, s1);
        while ((s = va_arg(args, char *))) {
            l = strlen(s);
            if ((m += l) < l) {
                m = SIZE_MAX;
                break;
            }
            if (m < dstsize) {
                memcpy(p, s, l);
                p += l;
            }
        }
        va_end(args);
        *p = '\0';
    } else {
        va_start(args, s1);
        while ((s = va_arg(args, char *))) {
            l = strlen(s);
            if ((m += l) < l) {
                m = SIZE_MAX;
                break;
            }
        }
        va_end(args);
    }
    return m;
}

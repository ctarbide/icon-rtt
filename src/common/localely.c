/* automatically generated from localely.nw (also available)
 */

/* Author: Richard James Howe
 * License: The Unlicense
 * Email: howe.r.j.89@gmail.com
 * Repo: https://github.com/howerj/localely
 *
 * As you may know, "locale.h" should be avoided, for a good
 * reason why, see:
 *
 * <https://github.com/mpv-player/mpv/commit/1e70e82baa9193f6f027338b0fab0f5078971fbe>.
 *
 * The functions in <ctype.h> are locale dependent. This means you can
 * get unexpected behavior with those functions (especially if some random
 * library goes and uses "setlocale" within your program).
 *
 * If you want a set of portable isX() functions in C that does not
 * depend on the locale, look no further, or look further if you want
 * super highly optimized code.
 *
 * We could add a few other functions for printing and scanning numbers.
 *
 * Ideally we would also make a set of printf/scanf functions that were
 * also locale independent as well, but those functions are are quite
 * complex.
 *
 * This library deals with ASCII/Bytes, not UTF-8 or anything else.
 *
 * Please see the repository link above for the full original text.
 *
 */
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

int C_isascii(int ch);
int C_isspace(int ch);
int C_iscntrl(int ch);
int C_isprint(int ch);
int C_isblank(int ch);
int C_isgraph(int ch);
int C_isupper(int ch);
int C_islower(int ch);
int C_isalpha(int ch);
int C_isdigit(int ch);
int C_isalnum(int ch);
int C_ispunct(int ch);
int C_isxdigit(int ch);
int C_toupper(int ch);
int C_tolower(int ch);

int C_isascii(int ch)
   {
   int r;
   r = (ch < 128 && ch >= 0);
   return r;
   }
int C_isspace(int ch)
   {
   int r;
   r = (ch >= 9 && ch <= 13) || ch == 32;
   return r;
   }
int C_iscntrl(int ch)
   {
   int r;
   r = (ch < 32 || ch == 127) && (ch < 128 && ch >= 0);
   return r;
   }
int C_isprint(int ch)
   {
   int r;
   r = !(ch < 32 || ch == 127) && (ch < 128 && ch >= 0);
   return r;
   }
int C_isblank(int ch)
   {
   int r;
   r = ch == 32 || ch == 9;
   return r;
   }
int C_isgraph(int ch)
   {
   int r;
   r = ch > 32 && ch < 127;
   return r;
   }
int C_isupper(int ch)
   {
   int r;
   r = (ch >= 65 && ch <= 90);
   return r;
   }
int C_islower(int ch)
   {
   int r;
   r = (ch >= 97 && ch <= 122);
   return r;
   }
int C_isalpha(int ch)
   {
   int r;
   r = ((ch >= 97 && ch <= 122) || (ch >= 65 && ch <= 90));
   return r;
   }
int C_isdigit(int ch)
   {
   int r;
   r = (ch >= 48 && ch <= 57);
   return r;
   }
int C_isalnum(int ch)
   {
   int r;
   r = ((ch >= 97 && ch <= 122) || (ch >= 65 && ch <= 90)) || (ch >= 48 && ch <= 57);
   return r;
   }
int C_ispunct(int ch)
   {
   int r;
   r = (ch >= 33 && ch <= 47) || (ch >= 58 && ch <= 64) || (ch >= 91 && ch <= 96) || (ch >= 123 && ch <= 126);
   return r;
   }
int C_isxdigit(int ch)
   {
   int r;
   r = (ch >= 65 && ch <= 70) || (ch >= 97 && ch <= 102) || (ch >= 48 && ch <= 57);
   return r;
   }
int C_toupper(int ch)
   {
   return (ch >= 97 && ch <= 122) ? ch ^ 0x20 : ch;
   }
int C_tolower(int ch)
   {
   return (ch >= 65 && ch <= 90) ? ch ^ 0x20 : ch;
   }

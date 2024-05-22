/*
 * Icon configuration file for Linux with MUSL C library
 */

#define UNIX 1
#define LoadFunc

#define __ANSI_C_PEDANTIC__

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

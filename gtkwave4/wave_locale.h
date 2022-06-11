#ifndef __WAVE_LOCALE_H__
#define __WAVE_LOCALE_H__

#include "config.h"

#ifdef __MINGW32__
#include "locale.h"
#define WAVE_LOCALE_FIX {setlocale(LC_ALL, "C"); }
#else
#if HAVE_SETENV && HAVE_UNSETENV
#define WAVE_LOCALE_FIX \
{ \
char *wlve = getenv("LANG"); \
if(wlve) \
        { \
        if(strcmp(wlve, "C")) \
                { \
                setenv("LC_NUMERIC", "C", 1); \
                setenv("LC_COLLATE", "C", 1); \
                setenv("LC_CTYPE", "C", 1); \
                } \
        } \
wlve = getenv("LC_ALL"); \
if(wlve) \
        { \
        if(strcmp(wlve, "C")) \
                { \
                unsetenv("LC_ALL"); \
                } \
        } \
}
#else
#define WAVE_LOCALE_FIX \
{ \
putenv(strdup("LANG=C")); \
putenv(strdup("LC_NUMERIC=C")); \
putenv(strdup("LC_COLLATE=C")); \
putenv(strdup("LC_CTYPE=C")); \
putenv(strdup("LC_ALL=C")); \
}
#endif
#endif

#endif /* __WAVE_LOCALE_H__*/


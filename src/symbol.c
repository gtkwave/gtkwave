/*
 * Copyright (c) Tony Bybell 2001-2010
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "globals.h"
#include <stdio.h>

#include <unistd.h>
#ifndef __MINGW32__
#include <sys/mman.h>
#endif

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "symbol.h"
#include "vcd.h"
#include "bsearch.h"

#ifdef _WAVE_HAVE_JUDY
#include <Judy.h>
#endif

/*
 * s_selected accessors
 */

// TODO: reenable judy support
// #ifdef _WAVE_HAVE_JUDY
//
// char get_s_selected(GwSymbol *s)
// {
//     int rc = Judy1Test(GLOBALS->s_selected, (Word_t)s, PJE0);
//
//     return (rc);
// }
//
// char set_s_selected(GwSymbol *s, char value)
// {
//     if (value) {
//         Judy1Set((Pvoid_t)&GLOBALS->s_selected, (Word_t)s, PJE0);
//     } else {
//         Judy1Unset((Pvoid_t)&GLOBALS->s_selected, (Word_t)s, PJE0);
//     }
//
//     return (value);
// }
//
// void destroy_s_selected(void)
// {
//     Judy1FreeArray(&GLOBALS->s_selected, PJE0);
//
//     GLOBALS->s_selected = NULL;
// }
//
// #else

char get_s_selected(GwSymbol *s)
{
    return (s->s_selected);
}

char set_s_selected(GwSymbol *s, char value)
{
    return ((s->s_selected = value));
}

void destroy_s_selected(void)
{
    /* nothing */
}

// #endif

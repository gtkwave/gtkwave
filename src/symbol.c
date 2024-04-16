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

/*
 * find a slot already in the table...
 */
static GwSymbol *symfind_2(char *s, unsigned int *rows_return)
{
    GwSymbol *sr;
    DEBUG(printf("BSEARCH: %s\n", s));

    sr = bsearch_facs(s, rows_return);
    if (sr) {
    } else {
        /* this is because . is > in ascii than chars like $ but . was converted to 0x1 on sort
         */
        char *s2;
        int i;
        int mat;

        gboolean has_escaped_names = gw_dump_file_has_escaped_names(GLOBALS->dump_file);
        if (!has_escaped_names) {
            return (sr);
        }

        GwFacs *facs = gw_dump_file_get_facs(GLOBALS->dump_file);
        guint numfacs = gw_facs_get_length(facs);

        if (GLOBALS->facs_have_symbols_state_machine == 0) {
            if (has_escaped_names) {
                mat = 1;
            } else {
                mat = 0;

                for (i = 0; i < numfacs; i++) {
                    char *hfacname = NULL;

                    GwSymbol *fac = gw_facs_get(facs, i);

                    hfacname = fac->name;
                    s2 = hfacname;
                    while (*s2) {
                        if (*s2 < GLOBALS->hier_delimeter) {
                            mat = 1;
                            break;
                        }
                        s2++;
                    }

                    /* if(was_packed) { free_2(hfacname); } ...not needed with
                     * HIER_DEPACK_STATIC */
                    if (mat) {
                        break;
                    }
                }
            }

            if (mat) {
                GLOBALS->facs_have_symbols_state_machine = 1;
            } else {
                GLOBALS->facs_have_symbols_state_machine = 2;
            } /* prevent code below from executing */
        }

        if (GLOBALS->facs_have_symbols_state_machine == 1) {
            mat = 0;
            for (i = 0; i < numfacs; i++) {
                char *hfacname = NULL;

                GwSymbol *fac = gw_facs_get(facs, i);

                hfacname = fac->name;
                if (!strcmp(hfacname, s)) {
                    mat = 1;
                }

                /* if(was_packed) { free_2(hfacname); } ...not needed with HIER_DEPACK_STATIC */
                if (mat) {
                    sr = fac;
                    break;
                }
            }
        }
    }

    return (sr);
}

GwSymbol *symfind(char *s, unsigned int *rows_return)
{
    GwSymbol *s_pnt = symfind_2(s, rows_return);

    if (!s_pnt) {
        int len = strlen(s);
        if (len) {
            char ch = s[len - 1];
            if ((ch != ']') && (ch != '}')) {
                char *s2 = g_alloca(len + 4);
                memcpy(s2, s, len);
                strcpy(s2 + len, "[0]"); /* bluespec vs modelsim */

                s_pnt = symfind_2(s2, rows_return);
            }
        }
    }

    return (s_pnt);
}

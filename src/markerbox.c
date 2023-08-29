/*
 * Copyright (c) Tony Bybell 1999-2016.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include <string.h>
#include "gtk23compat.h"
#include "debug.h"
#include "analyzer.h"
#include "currenttime.h"

static void gtkwave_strrev(char *p)
{
    char *q = p;
    while (q && *q)
        ++q;
    for (--q; p < q; ++p, --q)
        *p = *p ^ *q, *q = *p ^ *q, *p = *p ^ *q;
}

unsigned int bijective_marker_id_string_hash(const char *so)
{
    unsigned int val = 0;
    int i;
    int len = strlen(so);
    char sn[16];
    char *s = sn;

    strcpy(sn, so);
    gtkwave_strrev(sn);

    s += len;
    for (i = 0; i < len; i++) {
        char c = toupper(*(--s));
        if ((c < 'A') || (c > 'Z'))
            break;
        val *= ('Z' - 'A' + 1);
        val += ((unsigned char)c) - ('A' - 1);
    }

    val--; /* bijective values start at one so decrement */
    return (val);
}

unsigned int bijective_marker_id_string_len(const char *s)
{
    int len = 0;

    while (*s) {
        char c = toupper(*s);
        if ((c >= 'A') && (c <= 'Z')) {
            len++;
            s++;
            continue;
        } else {
            break;
        }
    }

    return (len);
}


/*
 * Copyright (c) Tony Bybell 1999-2008
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include "symbol.h"
#include "lx2.h"
#include "gw-ghw-file.h"

void status_text(const char *str)
{
    if (GLOBALS->quiet_checkmenu) {
        /* when gtkwave_mlist_t check menuitems are being initialized */
        return;
    }

    int len = strlen(str);
    char ch = len ? str[len - 1] : 0;

    fprintf(stderr, "GTKWAVE | %s%s", str, (ch == '\n') ? "" : "\n");
}

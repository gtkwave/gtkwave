/*
 * Copyright (c) Tony Bybell 2006.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "globals.h"
#include "gtk12compat.h"

/*
 * gtk1 users are forced to use the old treesearch widget
 * for now, sorry.
 */
#if WAVE_USE_GTK2
#include "treesearch_gtk2.c"
#else
#include "treesearch_gtk1.c"
#endif

void mkmenu_treesearch_cleanup(GtkWidget *widget, gpointer data)
{
(void)widget;
(void)data;

/* nothing */
}


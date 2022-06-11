/*
 * Copyright (c) Tony Bybell 2006.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef WAVE_BUSYWIN_H
#define WAVE_BUSYWIN_H

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"

/* number of histents to create before kicking off gtk_main_iteration() checking */
#define WAVE_BUSY_ITER (1000)

void init_busy(void);
void set_window_busy_no_refresh(GtkWidget *w);
void set_window_busy(GtkWidget *w);
void set_window_idle(GtkWidget *w);
void busy_window_refresh(void);

void gtkwave_main_iteration(void);
void gtk_events_pending_gtk_main_iteration(void);
gboolean in_main_iteration(void);
gboolean ignore_context_swap(void);

#endif


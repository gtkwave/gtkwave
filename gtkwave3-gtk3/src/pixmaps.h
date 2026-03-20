/*
 * Copyright (c) Tony Bybell 1999-2026.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef WAVE_PIXMAPS_H
#define WAVE_PIXMAPS_H

#include <gtk/gtk.h>

#ifdef MAC_INTEGRATION
GdkPixbuf *
#else
void
#endif
make_pixmaps(GtkWidget *window);


#define WAVE_SPLASH_X (512)
#define WAVE_SPLASH_Y (384)

void make_splash_pixmaps(GtkWidget *window);
GdkPixbuf *XXX_gdk_pixbuf_new_from_xpm_data(const gchar **src);

#endif


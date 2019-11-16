/*
 * Copyright (c) Tony Bybell 1999-2011.
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

#ifdef WAVE_USE_GTK2

#define WAVE_SPLASH_X (512)
#define WAVE_SPLASH_Y (384)

void make_splash_pixmaps(GtkWidget *window);

#endif

#endif


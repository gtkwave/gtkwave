/*
 * Copyright (c) Tony Bybell 2008
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVEFONTENGINE_H
#define WAVEFONTENGINE_H

#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN)
#include <pango/pango.h>
#endif

#include <gtk/gtk.h>


struct font_engine_font_t
{
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN)
PangoFontDescription *desc;
PangoFont *font;
PangoFontMetrics *metrics;
#endif

int ascent, descent;
int mono_width;

GdkFont *gdkfont;

unsigned is_pango : 1;
unsigned is_mono : 1;
};

void load_all_fonts(void);

void font_engine_draw_string
			(GdkDrawable      		*drawable,
                         struct font_engine_font_t 	*font,
                         GdkGC            		*gc,
                         gint              		x,
                         gint              		y,
                         const gchar      		*string);

gint font_engine_string_measure
			(struct font_engine_font_t      *font,
                         const gchar    		*string);


#endif


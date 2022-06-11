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

#include <pango/pango.h>
#include <gtk/gtk.h>
#include <color.h>

struct font_engine_font_t
{
PangoFontDescription *desc;
PangoFont *font;
PangoFontMetrics *metrics;

int ascent, descent;
int mono_width;

/* GdkFont *gdkfont; */

unsigned is_mono : 1;
};

void load_all_fonts(void);

gint font_engine_string_measure
			(struct font_engine_font_t      *font,
                         const gchar    		*string);


void XXX_font_engine_draw_string
                        (cairo_t                        *cr,
                         struct font_engine_font_t	*font,
                         wave_rgb_t                     *gc,
                         gint                           x,
                         gint                           y,
                         const gchar                    *string);


#endif


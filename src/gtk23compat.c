/*
 * Copyright (c) Tony Bybell 2018.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "globals.h"
#include "gtk23compat.h"

#if GTK_CHECK_VERSION(3, 0, 0)

#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
cairo_t *XXX_gdk_cairo_create(GdkWindow *window, GdkDrawingContext **gdc)
{
    cairo_region_t *region = gdk_window_get_visible_region(window);
    GdkDrawingContext *context = gdk_window_begin_draw_frame(window, region);
    *gdc = context;
    cairo_region_destroy(region);

    cairo_t *cr = gdk_drawing_context_get_cairo_context(context);

    return (cr);
}
#endif

#ifdef WAVE_ALLOW_GTK3_SEAT_VS_POINTER_GRAB_UNGRAB
void XXX_gdk_pointer_ungrab(guint32 time_)
{
    (void)time_;

    GdkDisplay *display = gdk_display_get_default();
    GdkSeat *seat = gdk_display_get_default_seat(display);
    gdk_seat_ungrab(seat);
}
#endif

#endif

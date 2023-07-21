/*
 * Copyright (c) Tony Bybell 2010
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_ZOOMBUTTONS_H
#define WAVE_ZOOMBUTTONS_H

void fix_wavehadj(void);
void service_zoom_in(GtkWidget *text, gpointer data);
void service_zoom_out(GtkWidget *text, gpointer data);
void service_zoom_fit(GtkWidget *text, gpointer data);
void service_zoom_full(GtkWidget *text, gpointer data);
void service_zoom_undo(GtkWidget *text, gpointer data);
void service_zoom_left(GtkWidget *text, gpointer data);
void service_zoom_right(GtkWidget *text, gpointer data);
void service_dragzoom(TimeType time1, TimeType time2);

#endif


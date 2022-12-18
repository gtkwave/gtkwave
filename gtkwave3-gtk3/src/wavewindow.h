/*
 * Copyright (c) Tony Bybell 2010
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_WAVEWINDOW_H
#define WAVE_WAVEWINDOW_H

void button_press_release_common(void);
void UpdateSigValue(Trptr t);
void MaxSignalLength(void);
void MaxSignalLength_2(char dirty_kick); /* used to resize but not fully recalculate like MaxSignalLength() */

void RenderSigs(int trtarget, int update_waves);
int RenderSig(Trptr t, int i, int dobackground);
void populateBuffer(Trptr t, char *altname, char* buf);
void calczoom(double z0);
void make_sigarea_gcs(GtkWidget *widget);
void force_screengrab_gcs(void);
void force_normal_gcs(void);
gint wavearea_configure_event(GtkWidget *widget, GdkEventConfigure *event);

void XXX_gdk_draw_line(cairo_t *cr, wave_rgb_t gc, gint _x1, gint _y1, gint _x2, gint _y2);
void XXX_gdk_draw_rectangle(cairo_t *cr, wave_rgb_t gc, gboolean filled, gint _x1, gint _y1, gint _w, gint _h);

#ifdef WAVE_GTK3_SIZE_ALLOCATE_WORKAROUND_WAVE_VSLIDER
void service_vslider(GtkWidget *text, gpointer data);
#endif

#endif


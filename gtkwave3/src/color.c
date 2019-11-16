/*
 * Copyright (c) Tony Bybell 1999-2011.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include "color.h"
#include "debug.h"

/*
 * return graphics context with tuple's color or
 * a fallback context.  Note that if tuple<0,
 * the fallback will be used!
 */
GdkGC *alloc_color(GtkWidget *widget, int tuple, GdkGC *fallback)
{
GdkColor color;
GdkGC    *gc;
int red, green, blue;

red=  (tuple>>16)&0x000000ff;
green=(tuple>>8) &0x000000ff;
blue= (tuple)    &0x000000ff;

if(tuple>=0)
if((gc=gdk_gc_new(widget->window)))
	{
	struct wave_gcchain_t *wg = calloc_2(1, sizeof(struct wave_gcchain_t));

	color.red=red*(65535/255);
	color.blue=blue*(65535/255);
	color.green=green*(65535/255);
	color.pixel=(gulong)(tuple&0x00ffffff);
	gdk_color_alloc(gtk_widget_get_colormap(widget),&color);
	gdk_gc_set_foreground(gc,&color);

	wg->next = GLOBALS->wave_gcchain; /* remember allocated ones only, not fallbacks */
	wg->gc = gc;
	GLOBALS->wave_gcchain = wg;

	return(gc);
	}

return(fallback);
}


void dealloc_all_gcs(void)
{
struct wave_gcchain_t *wg = GLOBALS->wave_gcchain;

while(wg)
	{
	if(wg->gc)
		{
		gdk_gc_destroy(wg->gc);
		wg->gc = NULL;
		}

	wg = wg->next;
	}
}


void set_alternate_gcs(GdkGC *ctx, GdkGC *ctx_fill)
{
GLOBALS->gc.gc_low_wavewindow_c_1 = ctx;
GLOBALS->gc.gc_high_wavewindow_c_1 = ctx;
GLOBALS->gc.gc_trans_wavewindow_c_1 = ctx;
GLOBALS->gc.gc_0_wavewindow_c_1 = ctx;
GLOBALS->gc.gc_1_wavewindow_c_1 = ctx;
GLOBALS->gc.gc_vbox_wavewindow_c_1 = ctx;
GLOBALS->gc.gc_vtrans_wavewindow_c_1 = ctx;

if(!GLOBALS->keep_xz_colors)
	{
	GLOBALS->gc.gc_mid_wavewindow_c_1 = ctx;
	GLOBALS->gc.gc_highfill_wavewindow_c_1 = ctx_fill;
	GLOBALS->gc.gc_1fill_wavewindow_c_1 = ctx_fill;
	GLOBALS->gc.gc_xfill_wavewindow_c_1 = ctx_fill;
	GLOBALS->gc.gc_x_wavewindow_c_1 = ctx;
	GLOBALS->gc.gc_ufill_wavewindow_c_1 = ctx_fill;
	GLOBALS->gc.gc_u_wavewindow_c_1 = ctx;
	GLOBALS->gc.gc_wfill_wavewindow_c_1 = ctx_fill;
	GLOBALS->gc.gc_w_wavewindow_c_1 = ctx;
	GLOBALS->gc.gc_dashfill_wavewindow_c_1 = ctx_fill;
	GLOBALS->gc.gc_dash_wavewindow_c_1 = ctx;
	}
}

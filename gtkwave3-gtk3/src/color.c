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
 * return graphics context with tuple's color
 */
wave_rgb_t XXX_alloc_color(int tuple)
{
wave_rgb_t rc;
int red, green, blue;

red=  (tuple>>16)&0x000000ff;
green=(tuple>>8) &0x000000ff;
blue= (tuple)    &0x000000ff;

rc.r = (double)red/255;
rc.g = (double)green/255;
rc.b = (double)blue/255;
rc.a = (double)1.0;

return(rc);
}



void XXX_set_alternate_gcs(wave_rgb_t ctx, wave_rgb_t ctx_fill)
{
GLOBALS->rgb_gc.gc_low_wavewindow_c_1 = ctx;
GLOBALS->rgb_gc.gc_high_wavewindow_c_1 = ctx;
GLOBALS->rgb_gc.gc_trans_wavewindow_c_1 = ctx;
GLOBALS->rgb_gc.gc_0_wavewindow_c_1 = ctx;
GLOBALS->rgb_gc.gc_1_wavewindow_c_1 = ctx;
GLOBALS->rgb_gc.gc_vbox_wavewindow_c_1 = ctx;
GLOBALS->rgb_gc.gc_vtrans_wavewindow_c_1 = ctx;

if(!GLOBALS->keep_xz_colors)
	{
	GLOBALS->rgb_gc.gc_mid_wavewindow_c_1 = ctx;
	GLOBALS->rgb_gc.gc_highfill_wavewindow_c_1 = ctx_fill;
	GLOBALS->rgb_gc.gc_1fill_wavewindow_c_1 = ctx_fill;
	GLOBALS->rgb_gc.gc_xfill_wavewindow_c_1 = ctx_fill;
	GLOBALS->rgb_gc.gc_x_wavewindow_c_1 = ctx;
	GLOBALS->rgb_gc.gc_ufill_wavewindow_c_1 = ctx_fill;
	GLOBALS->rgb_gc.gc_u_wavewindow_c_1 = ctx;
	GLOBALS->rgb_gc.gc_wfill_wavewindow_c_1 = ctx_fill;
	GLOBALS->rgb_gc.gc_w_wavewindow_c_1 = ctx;
	GLOBALS->rgb_gc.gc_dashfill_wavewindow_c_1 = ctx_fill;
	GLOBALS->rgb_gc.gc_dash_wavewindow_c_1 = ctx;
	}
}


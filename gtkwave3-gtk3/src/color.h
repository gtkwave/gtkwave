/*
 * Copyright (c) Tony Bybell 1999-2011.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef WAVE_COLOR_H
#define WAVE_COLOR_H

#include <stdlib.h>
#include <gtk/gtk.h>

#define WAVE_COLOR_CYCLE (-1)
#define WAVE_COLOR_NORMAL (0)
#define WAVE_COLOR_RED (1)
#define WAVE_COLOR_ORANGE (2)
#define WAVE_COLOR_YELLOW (3)
#define WAVE_COLOR_GREEN (4)
#define WAVE_COLOR_BLUE (5)
#define WAVE_COLOR_INDIGO (6)
#define WAVE_COLOR_VIOLET (7)

#define WAVE_NUM_RAINBOW (7)

#define WAVE_RAINBOW_RGB {0xFF0000, 0xFF7F00, 0xFFFF00, 0x00FF00, 0x0000FF, 0x6600FF, 0x9B00FF}

#define RGB_WAVE_RAINBOW_INITIALIZER {{0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0}}

struct wave_rgb_t
{
double r, g, b, a;
};

typedef struct wave_rgb_t wave_rgb_t;

struct wave_rgbmaster_t
{
wave_rgb_t gc_ltgray;
wave_rgb_t gc_normal;
wave_rgb_t gc_mdgray;
wave_rgb_t gc_dkgray;
wave_rgb_t gc_dkblue;
wave_rgb_t gc_brkred;
wave_rgb_t gc_ltblue;
wave_rgb_t gc_gmstrd;
wave_rgb_t gc_back_wavewindow_c_1;
wave_rgb_t gc_baseline_wavewindow_c_1;
wave_rgb_t gc_grid_wavewindow_c_1;
wave_rgb_t gc_grid2_wavewindow_c_1;
wave_rgb_t gc_time_wavewindow_c_1;
wave_rgb_t gc_timeb_wavewindow_c_1;
wave_rgb_t gc_value_wavewindow_c_1;
wave_rgb_t gc_low_wavewindow_c_1;
wave_rgb_t gc_highfill_wavewindow_c_1;
wave_rgb_t gc_high_wavewindow_c_1;
wave_rgb_t gc_trans_wavewindow_c_1;
wave_rgb_t gc_mid_wavewindow_c_1;
wave_rgb_t gc_xfill_wavewindow_c_1;
wave_rgb_t gc_x_wavewindow_c_1;
wave_rgb_t gc_vbox_wavewindow_c_1;
wave_rgb_t gc_vtrans_wavewindow_c_1;
wave_rgb_t gc_mark_wavewindow_c_1;
wave_rgb_t gc_umark_wavewindow_c_1;
wave_rgb_t gc_0_wavewindow_c_1;
wave_rgb_t gc_1fill_wavewindow_c_1;
wave_rgb_t gc_1_wavewindow_c_1;
wave_rgb_t gc_ufill_wavewindow_c_1;
wave_rgb_t gc_u_wavewindow_c_1;
wave_rgb_t gc_wfill_wavewindow_c_1;
wave_rgb_t gc_w_wavewindow_c_1;
wave_rgb_t gc_dashfill_wavewindow_c_1;
wave_rgb_t gc_dash_wavewindow_c_1;
};

void XXX_set_alternate_gcs(wave_rgb_t ctx, wave_rgb_t ctx_fill);
wave_rgb_t XXX_alloc_color(int tuple);


#endif


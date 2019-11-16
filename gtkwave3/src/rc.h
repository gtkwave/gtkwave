/*
 * Copyright (c) Tony Bybell 1999-2016.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef WAVE_RC_H
#define WAVE_RC_H

struct rc_entry
{
char *name;
int (*func)(char *);
};

struct rc_override /* used for --rcvar command line option */
{
struct rc_override *next;
char *str;
};

void read_rc_file(char *override_rc);
int insert_rc_variable(char *str);
int get_rgb_from_name(char *str);
GdkGC *get_gc_from_name(char *str);

int  f_accel (char *str);
int  f_alt_hier_delimeter (char *str);
int  f_append_vcd_hier (char *str);
int  f_atomic_vectors (char *str);
int  f_autocoalesce (char *str);
int  f_autocoalesce_reversal (char *str);
int  f_autoname_bundles (char *str);
int  f_color_0 (char *str);
int  f_color_1 (char *str);
int  f_color_1fill (char *str);
int  f_color_back (char *str);
int  f_color_baseline (char *str);
int  f_color_black (char *str);
int  f_color_dash (char *str);
int  f_color_dashfill (char *str);
int  f_color_dkblue (char *str);
int  f_color_brkred (char *str);
int  f_color_ltblue (char *str);
int  f_color_gmstrd (char *str);
int  f_color_dkgray (char *str);
int  f_color_grid (char *str);
int  f_color_grid2 (char *str);
int  f_color_high (char *str);
int  f_color_highfill (char *str);
int  f_color_low (char *str);
int  f_color_ltgray (char *str);
int  f_color_mark (char *str);
int  f_color_mdgray (char *str);
int  f_color_mid (char *str);
int  f_color_normal (char *str);
int  f_color_time (char *str);
int  f_color_timeb (char *str);
int  f_color_trans (char *str);
int  f_color_u (char *str);
int  f_color_ufill (char *str);
int  f_color_umark (char *str);
int  f_color_value (char *str);
int  f_color_vbox (char *str);
int  f_color_vtrans (char *str);
int  f_color_w (char *str);
int  f_color_wfill (char *str);
int  f_color_white (char *str);
int  f_color_x (char *str);
int  f_color_xfill (char *str);
int  f_constant_marker_update (char *str);
int  f_convert_to_reals (char *str);
int  f_cursor_snap (char *str);
int  f_clipboard_mouseover (char *str);
int  f_disable_mouseover (char *str);
int  f_disable_tooltips (char *str);
int  f_do_initial_zoom_fit (char *str);
int  f_dynamic_resizing (char *str);
int  f_enable_fast_exit (char *str);
int  f_enable_ghost_marker (char *str);
int  f_enable_horiz_grid (char *str);
int  f_enable_vcd_autosave (char *str);
int  f_enable_vert_grid (char *str);
int  f_fill_waveform (char *str);
int  f_fontname_logfile (char *str);
int  f_fontname_signals (char *str);
int  f_fontname_waves (char *str);
int  f_force_toolbars (char *str);
int  f_hide_sst (char *str);
int  f_hier_delimeter (char *str);
int  f_hier_grouping (char *str);
int  f_hier_max_level (char *str);
int  f_hpane_pack (char *str);
int  f_ignore_savefile_pos (char *str);
int  f_ignore_savefile_size (char *str);
int  f_initial_window_x (char *str);
int  f_initial_window_xpos (char *str);
int  f_initial_window_y (char *str);
int  f_initial_window_ypos (char *str);
int  f_left_justify_sigs (char *str);
int  f_lxt_clock_compress_to_z (char *str);
int  f_page_divisor (char *str);
int  f_ps_maxveclen (char *str);
int  f_show_base_symbols (char *str);
int  f_show_grid (char *str);
int  f_splash_disable (char *str);
int  f_sst_expanded (char *str);
int  f_use_big_fonts (char *str);
int  f_use_frequency_display (char *str);
int  f_use_full_precision (char *str);
int  f_use_maxtime_display (char *str);
int  f_use_nonprop_fonts (char *str);
int  f_use_roundcaps (char *str);
int  f_use_scrollbar_only (char *str);
int  f_vcd_explicit_zero_subscripts (char *str);
int  f_vcd_preserve_glitches (char *str);
int  f_vcd_warning_filesize (char *str);
int  f_vector_padding (char *str);
int  f_vlist_compression (char *str);
int  f_wave_scrolling (char *str);
int  f_zoom_base (char *str);
int  f_zoom_center (char *str);
int  f_zoom_pow10_snap (char *str);

#endif

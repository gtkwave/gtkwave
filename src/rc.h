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
    const char *name;
    int (*func)(const char *);
};

struct rc_override /* used for --rcvar command line option */
{
    struct rc_override *next;
    char *str;
};

void read_rc_file(const char *override_rc);
int insert_rc_variable(char *str);

int f_accel(const char *str);
int f_alt_hier_delimeter(const char *str);
int f_autocoalesce(const char *str);
int f_autocoalesce_reversal(const char *str);
int f_autoname_bundles(const char *str);
int f_color_0(const char *str);
int f_color_1(const char *str);
int f_color_1fill(const char *str);
int f_color_back(const char *str);
int f_color_baseline(const char *str);
int f_color_black(const char *str);
int f_color_dash(const char *str);
int f_color_dashfill(const char *str);
int f_color_dkblue(const char *str);
int f_color_brkred(const char *str);
int f_color_ltblue(const char *str);
int f_color_gmstrd(const char *str);
int f_color_dkgray(const char *str);
int f_color_grid(const char *str);
int f_color_grid2(const char *str);
int f_color_high(const char *str);
int f_color_highfill(const char *str);
int f_color_low(const char *str);
int f_color_ltgray(const char *str);
int f_color_mark(const char *str);
int f_color_mdgray(const char *str);
int f_color_mid(const char *str);
int f_color_normal(const char *str);
int f_color_time(const char *str);
int f_color_timeb(const char *str);
int f_color_trans(const char *str);
int f_color_u(const char *str);
int f_color_ufill(const char *str);
int f_color_umark(const char *str);
int f_color_value(const char *str);
int f_color_vbox(const char *str);
int f_color_w(const char *str);
int f_color_wfill(const char *str);
int f_color_white(const char *str);
int f_color_x(const char *str);
int f_color_xfill(const char *str);
int f_constant_marker_update(const char *str);
int f_convert_to_reals(const char *str);
int f_cursor_snap(const char *str);
int f_clipboard_mouseover(const char *str);
int f_disable_mouseover(const char *str);
int f_disable_tooltips(const char *str);
int f_do_initial_zoom_fit(const char *str);
int f_dynamic_resizing(const char *str);
int f_enable_fast_exit(const char *str);
int f_enable_ghost_marker(const char *str);
int f_enable_horiz_grid(const char *str);
int f_enable_vcd_autosave(const char *str);
int f_enable_vert_grid(const char *str);
int f_fill_waveform(const char *str);
int f_fontname_logfile(const char *str);
int f_fontname_signals(const char *str);
int f_fontname_waves(const char *str);
int f_hier_delimeter(const char *str);
int f_hier_max_level(const char *str);
int f_ignore_savefile_pos(const char *str);
int f_ignore_savefile_size(const char *str);
int f_initial_window_x(const char *str);
int f_initial_window_xpos(const char *str);
int f_initial_window_y(const char *str);
int f_initial_window_ypos(const char *str);
int f_left_justify_sigs(const char *str);
int f_lxt_clock_compress_to_z(const char *str);
int f_page_divisor(const char *str);
int f_ps_maxveclen(const char *str);
int f_show_base_symbols(const char *str);
int f_show_grid(const char *str);
int f_splash_disable(const char *str);
int f_sst_expanded(const char *str);
int f_use_big_fonts(const char *str);
int f_use_frequency_display(const char *str);
int f_use_full_precision(const char *str);
int f_use_maxtime_display(const char *str);
int f_use_nonprop_fonts(const char *str);
int f_use_roundcaps(const char *str);
int f_vcd_preserve_glitches(const char *str);
int f_vcd_warning_filesize(const char *str);
int f_vector_padding(const char *str);
int f_vlist_compression(const char *str);
int f_wave_scrolling(const char *str);
int f_zoom_base(const char *str);
int f_zoom_center(const char *str);
int f_zoom_pow10_snap(const char *str);

#endif

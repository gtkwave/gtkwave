/*
 * Copyright (c) Tony Bybell 1999-2017.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

/* AIX may need this for alloca to work */
#if defined _AIX
#pragma alloca
#endif

#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include "analyzer.h"
#include "currenttime.h"
#include "symbol.h"
#include "vcd.h"
#include "fgetdynamic.h"
#include "debug.h"
#include "main.h"
#include "menu.h"
#include "color.h"
#include "vlist.h"
#include "rc.h"

#ifdef MAC_INTEGRATION
#include <gtkosxapplication.h>
#endif

#ifndef __MINGW32__
#include <unistd.h>
#include <pwd.h>
static const char *rcname = ".gtkwaverc"; /* name of environment file--POSIX */
#else
static const char *rcname = "gtkwave.ini"; /* name of environment file--WIN32 */
#endif

/*
 * functions that set the individual rc variables..
 */
int f_accel(const char *str)
{
    DEBUG(printf("f_accel(\"%s\")\n", str));

    if (strlen(str)) {
        set_wave_menu_accelerator(str);
    }

    return (0);
}

int f_alt_hier_delimeter(const char *str)
{
    DEBUG(printf("f_alt_hier_delimeter(\"%s\")\n", str));

    if (strlen(str)) {
        GLOBALS->alt_hier_delimeter = str[0];
    }
    return (0);
}

int f_analog_redraw_skip_count(const char *str)
{
    DEBUG(printf("f_analog_redraw_skip_count(\"%s\")\n", str));
    GLOBALS->analog_redraw_skip_count = atoi_64(str);
    if (GLOBALS->analog_redraw_skip_count < 0) {
        GLOBALS->analog_redraw_skip_count = 0;
    }

    return (0);
}

int f_autoname_bundles(const char *str)
{
    DEBUG(printf("f_autoname_bundles(\"%s\")\n", str));
    GLOBALS->autoname_bundles = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_autocoalesce(const char *str)
{
    DEBUG(printf("f_autocoalesce(\"%s\")\n", str));
    GLOBALS->autocoalesce = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_autocoalesce_reversal(const char *str)
{
    DEBUG(printf("f_autocoalesce_reversal(\"%s\")\n", str));
    GLOBALS->autocoalesce_reversal = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_constant_marker_update(const char *str)
{
    DEBUG(printf("f_constant_marker_update(\"%s\")\n", str));
    GLOBALS->constant_marker_update = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_context_tabposition(const char *str)
{
    DEBUG(printf("f_convert_to_reals(\"%s\")\n", str));
    GLOBALS->context_tabposition = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_convert_to_reals(const char *str)
{
    DEBUG(printf("f_convert_to_reals(\"%s\")\n", str));
    GLOBALS->convert_to_reals = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_cursor_snap(const char *str)
{
    int val;
    DEBUG(printf("f_cursor_snap(\"%s\")\n", str));
    val = atoi_64(str);
    GLOBALS->cursor_snap = (val <= 0) ? 0 : val;
    return (0);
}

int f_dbl_mant_dig_override(const char *str)
{
    DEBUG(printf("f_dbl_mant_dig_override(\"%s\")\n", str));
    GLOBALS->dbl_mant_dig_override = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_disable_antialiasing(const char *str)
{
    DEBUG(printf("f_disable_antialiasing(\"%s\")\n", str));
    GLOBALS->disable_antialiasing = atoi_64(str) ? TRUE : FALSE;
    return (0);
}

int f_disable_auto_comphier(const char *str)
{
    DEBUG(printf("f_disable_auto_comphier(\"%s\")\n", str));
    GLOBALS->disable_auto_comphier = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_disable_empty_gui(const char *str)
{
    DEBUG(printf("f_disable_empty_gui(\"%s\")\n", str));
    GLOBALS->disable_empty_gui = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_disable_mouseover(const char *str)
{
    DEBUG(printf("f_disable_mouseover(\"%s\")\n", str));
    GLOBALS->disable_mouseover = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_clipboard_mouseover(const char *str)
{
    DEBUG(printf("f_clipboard_mouseover(\"%s\")\n", str));
    GLOBALS->clipboard_mouseover = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_disable_tooltips(const char *str)
{
    DEBUG(printf("f_disable_tooltips(\"%s\")\n", str));
    GLOBALS->disable_tooltips = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_do_initial_zoom_fit(const char *str)
{
    DEBUG(printf("f_do_initial_zoom_fit(\"%s\")\n", str));
    GLOBALS->do_initial_zoom_fit = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_dragzoom_threshold(const char *str)
{
    DEBUG(printf("f_dragzoom_threshold(\"%s\")\n", str));
    GLOBALS->dragzoom_threshold = atoi_64(str);
    return (0);
}

int f_dynamic_resizing(const char *str)
{
    DEBUG(printf("f_dynamic_resizing(\"%s\")\n", str));
    GLOBALS->do_resize_signals = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_editor(const char *str)
{
    char *path, *pathend;

    DEBUG(printf("f_editor(\"%s\")\n", str));

    path = strchr(str, '\"');
    if (path) {
        path++;
        if (*path) {
            pathend = strchr(path, '\"');
            if (pathend) {
                *pathend = 0;
                if (GLOBALS->editor_name)
                    free_2(GLOBALS->editor_name);
                GLOBALS->editor_name = (char *)strdup_2(path);
            }
        }
    }

    return (0);
}

int f_editor_run_in_terminal(const char *str)
{
    DEBUG(printf("f_editor_run_in_terminal(\"%s\")\n", str));
    GLOBALS->editor_run_in_terminal = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_enable_fast_exit(const char *str)
{
    DEBUG(printf("f_enable_fast_exit(\"%s\")\n", str));
    GLOBALS->enable_fast_exit = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_enable_ghost_marker(const char *str)
{
    DEBUG(printf("f_enable_ghost_marker(\"%s\")\n", str));
    GLOBALS->enable_ghost_marker = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_enable_horiz_grid(const char *str)
{
    DEBUG(printf("f_enable_horiz_grid(\"%s\")\n", str));
    GLOBALS->enable_horiz_grid = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_enable_vcd_autosave(const char *str)
{
    DEBUG(printf("f_enable_vcd_autosave(\"%s\")\n", str));
    GLOBALS->make_vcd_save_file = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_enable_vert_grid(const char *str)
{
    DEBUG(printf("f_enable_vert_grid(\"%s\")\n", str));
    GLOBALS->enable_vert_grid = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_fill_waveform(const char *str)
{
    DEBUG(printf("f_fill_waveform(\"%s\")\n", str));
    GLOBALS->fill_waveform = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_lz_removal(const char *str)
{
    DEBUG(printf("f_lz_removal(\"%s\")\n", str));
    GLOBALS->lz_removal = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_fontname_logfile(const char *str)
{
    DEBUG(printf("f_fontname_logfile(\"%s\")\n", str));
    if (GLOBALS->fontname_logfile)
        free_2(GLOBALS->fontname_logfile);
    GLOBALS->fontname_logfile = (char *)malloc_2(strlen(str) + 1);
    strcpy(GLOBALS->fontname_logfile, str);
    return (0);
}

int f_fontname_signals(const char *str)
{
    DEBUG(printf("f_fontname_signals(\"%s\")\n", str));
    if (GLOBALS->fontname_signals)
        free_2(GLOBALS->fontname_signals);
    GLOBALS->fontname_signals = (char *)malloc_2(strlen(str) + 1);
    strcpy(GLOBALS->fontname_signals, str);
    return (0);
}

int f_fontname_waves(const char *str)
{
    DEBUG(printf("f_fontname_signals(\"%s\")\n", str));
    if (GLOBALS->fontname_waves)
        free_2(GLOBALS->fontname_waves);
    GLOBALS->fontname_waves = (char *)malloc_2(strlen(str) + 1);
    strcpy(GLOBALS->fontname_waves, str);
    return (0);
}

int f_hier_ignore_escapes(const char *str)
{
    DEBUG(printf("f_hier_ignore_escapes(\"%s\")\n", str));
    GLOBALS->hier_ignore_escapes = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_keep_xz_colors(const char *str)
{
    DEBUG(printf("f_keep_xz_colors(\"%s\")\n", str));
    GLOBALS->keep_xz_colors = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_sst_dbl_action_type(const char *str)
{
    DEBUG(printf("f_sst_dbl_action_type(\"%s\")\n", str));

    switch (str[0]) {
        case 'I':
        case 'i':
            GLOBALS->sst_dbl_action_type = SST_ACTION_INSERT;
            break;
        case 'R':
        case 'r':
            GLOBALS->sst_dbl_action_type = SST_ACTION_REPLACE;
            break;
        case 'A':
        case 'a':
            GLOBALS->sst_dbl_action_type = SST_ACTION_APPEND;
            break;
        case 'P':
        case 'p':
            GLOBALS->sst_dbl_action_type = SST_ACTION_PREPEND;
            break;

        default:
            GLOBALS->sst_dbl_action_type = SST_ACTION_NONE;
    }

    return (0);
}

int f_sst_dynamic_filter(const char *str)
{
    DEBUG(printf("f_sst_dynamic_filter(\"%s\")\n", str));
    GLOBALS->do_dynamic_treefilter = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_sst_expanded(const char *str)
{
    DEBUG(printf("f_sst_expanded(\"%s\")\n", str));
    GLOBALS->sst_expanded = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_hier_delimeter(const char *str)
{
    DEBUG(printf("f_hier_delimeter(\"%s\")\n", str));

    if (strlen(str)) {
        GLOBALS->hier_delimeter = str[0];
        GLOBALS->hier_was_explicitly_set = 1;
    }
    return (0);
}

int f_hier_max_level(const char *str)
{
    DEBUG(printf("f_hier_max_level(\"%s\")\n", str));
    GLOBALS->hier_max_level_shadow = GLOBALS->hier_max_level = atoi_64(str);
    return (0);
}

int f_highlight_wavewindow(const char *str)
{
    DEBUG(printf("f_highlight_wavewindow(\"%s\")\n", str));
    GLOBALS->highlight_wavewindow = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_ignore_savefile_pane_pos(const char *str)
{
    DEBUG(printf("f_ignore_savefile_pane_pos(\"%s\")\n", str));
    GLOBALS->ignore_savefile_pane_pos = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_ignore_savefile_pos(const char *str)
{
    DEBUG(printf("f_ignore_savefile_pos(\"%s\")\n", str));
    GLOBALS->ignore_savefile_pos = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_ignore_savefile_size(const char *str)
{
    DEBUG(printf("f_ignore_savefile_size(\"%s\")\n", str));
    GLOBALS->ignore_savefile_size = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_initial_signal_window_width(const char *str)
{
    int val;
    DEBUG(printf("f_initial_signal_window_width(\"%s\")\n", str));
    val = atoi_64(str);
    GLOBALS->initial_signal_window_width = (val < 0) ? 0 : val;
    return (0);
}

int f_initial_window_x(const char *str)
{
    int val;
    DEBUG(printf("f_initial_window_x(\"%s\")\n", str));
    val = atoi_64(str);
    GLOBALS->initial_window_x = (val <= 0) ? -1 : val;
    return (0);
}

int f_initial_window_xpos(const char *str)
{
    int val;
    DEBUG(printf("f_initial_window_xpos(\"%s\")\n", str));
    val = atoi_64(str);
    GLOBALS->initial_window_xpos = (val <= 0) ? -1 : val;
    return (0);
}

int f_initial_window_y(const char *str)
{
    int val;
    DEBUG(printf("f_initial_window_y(\"%s\")\n", str));
    val = atoi_64(str);
    GLOBALS->initial_window_y = (val <= 0) ? -1 : val;
    return (0);
}

int f_initial_window_ypos(const char *str)
{
    int val;
    DEBUG(printf("f_initial_window_ypos(\"%s\")\n", str));
    val = atoi_64(str);
    GLOBALS->initial_window_ypos = (val <= 0) ? -1 : val;
    return (0);
}

int f_left_justify_sigs(const char *str)
{
    DEBUG(printf("f_left_justify_sigs(\"%s\")\n", str));
    GLOBALS->left_justify_sigs = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_max_fsdb_trees(const char *str)
{
    int val;
    DEBUG(printf("f_max_fsdb_trees(\"%s\")\n", str));
    val = atoi_64(str);
    GLOBALS->extload_max_tree = (val < 0) ? 0 : val;
    return (0);
}

int f_page_divisor(const char *str)
{
    DEBUG(printf("f_page_divisor(\"%s\")\n", str));
    sscanf(str, "%lg", &GLOBALS->page_divisor);

    if (GLOBALS->page_divisor < 0.01) {
        GLOBALS->page_divisor = 0.01;
    } else if (GLOBALS->page_divisor > 100.0) {
        GLOBALS->page_divisor = 100.0;
    }

    if (GLOBALS->page_divisor > 1.0)
        GLOBALS->page_divisor = 1.0 / GLOBALS->page_divisor;

    return (0);
}

int f_ps_maxveclen(const char *str)
{
    DEBUG(printf("f_ps_maxveclen(\"%s\")\n", str));
    GLOBALS->ps_maxveclen = atoi_64(str);
    if (GLOBALS->ps_maxveclen < 4) {
        GLOBALS->ps_maxveclen = 4;
    } else if (GLOBALS->ps_maxveclen > 66) {
        GLOBALS->ps_maxveclen = 66;
    }

    return (0);
}

int f_scale_to_time_dimension(const char *str)
{
    int which = tolower((int)(*str));
    DEBUG(printf("f_scale_to_time_dimension(\"%s\")\n", str));

    if (strchr(WAVE_SI_UNITS, which) || (which == 's')) {
        GLOBALS->scale_to_time_dimension = which;
    } else {
        GLOBALS->scale_to_time_dimension = 0; /* also covers '*' case as not found above */
    }

    return (0);
}

int f_show_base_symbols(const char *str)
{
    DEBUG(printf("f_show_base_symbols(\"%s\")\n", str));
    GLOBALS->show_base = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_show_grid(const char *str)
{
    DEBUG(printf("f_show_grid(\"%s\")\n", str));
    GLOBALS->display_grid = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_splash_disable(const char *str)
{
    DEBUG(printf("f_splash_disable(\"%s\")\n", str));
    GLOBALS->splash_disable = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_strace_repeat_count(const char *str)
{
    DEBUG(printf("f_strace_repeat_count(\"%s\")\n", str));
    GLOBALS->strace_repeat_count = atoi_64(str);
    return (0);
}

int f_use_big_fonts(const char *str)
{
    DEBUG(printf("f_use_big_fonts(\"%s\")\n", str));
    GLOBALS->use_big_fonts = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_use_fat_lines(const char *str)
{
    DEBUG(printf("f_use_fat_lines(\"%s\")\n", str));
    GLOBALS->cr_line_width = atoi_64(str) ? 2.0 : 1.0;
    GLOBALS->cairo_050_offset = atoi_64(str) ? 0.0 : 0.5;
    return (0);
}

int f_use_frequency_display(const char *str)
{
    DEBUG(printf("f_use_frequency_display(\"%s\")\n", str));
    GLOBALS->use_frequency_delta = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_use_full_precision(const char *str)
{
    DEBUG(printf("f_use_full_precision(\"%s\")\n", str));
    GLOBALS->use_full_precision = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_use_gestures(const char *str)
{
    DEBUG(printf("f_use_gestures(\"%s\")\n", str));
    if (toupper(str[0]) == 'M') {
        GLOBALS->use_gestures = -1; /* maybe */
    } else {
        GLOBALS->use_gestures = atoi_64(str) ? 1 : 0;
    }
    return (0);
}

int f_use_maxtime_display(const char *str)
{
    DEBUG(printf("f_use_maxtime_display(\"%s\")\n", str));
    GLOBALS->use_maxtime_display = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_use_nonprop_fonts(const char *str)
{
    DEBUG(printf("f_use_nonprop_fonts(\"%s\")\n", str));
    GLOBALS->use_nonprop_fonts = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_use_pango_fonts(const char *str)
{
    DEBUG(printf("f_use_pango_fonts(\"%s\")\n", str));
    GLOBALS->use_pango_fonts = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_use_roundcaps(const char *str)
{
    DEBUG(printf("f_use_roundcaps(\"%s\")\n", str));
    GLOBALS->use_roundcaps = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_ruler_origin(const char *str)
{
    DEBUG(printf("f_ruler_origin(\"%s\")\n", str));
    GLOBALS->ruler_origin = atoi_64(str);
    return (0);
}

int f_ruler_step(const char *str)
{
    DEBUG(printf("f_ruler_step(\"%s\")\n", str));
    GLOBALS->ruler_step = atoi_64(str);
    return (0);
}

int f_vcd_preserve_glitches(const char *str)
{
    DEBUG(printf("f_vcd_preserve_glitches(\"%s\")\n", str));
    GLOBALS->vcd_preserve_glitches = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_vcd_preserve_glitches_real(const char *str)
{
    DEBUG(printf("f_vcd_preserve_glitches_real(\"%s\")\n", str));
    GLOBALS->vcd_preserve_glitches_real = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_vcd_warning_filesize(const char *str)
{
    DEBUG(printf("f_vcd_warning_filesize(\"%s\")\n", str));
    GLOBALS->vcd_warning_filesize = atoi_64(str);
    return (0);
}

int f_vector_padding(const char *str)
{
    DEBUG(printf("f_vector_padding(\"%s\")\n", str));
    GLOBALS->vector_padding = atoi_64(str);
    if (GLOBALS->vector_padding < 4)
        GLOBALS->vector_padding = 4;
    else if (GLOBALS->vector_padding > 16)
        GLOBALS->vector_padding = 16;
    return (0);
}

int f_vlist_compression(const char *str)
{
    DEBUG(printf("f_vlist_compression(\"%s\")\n", str));
    GLOBALS->vlist_compression_depth = atoi_64(str);
    if (GLOBALS->vlist_compression_depth < 0)
        GLOBALS->vlist_compression_depth = -1;
    if (GLOBALS->vlist_compression_depth > 9)
        GLOBALS->vlist_compression_depth = 9;
    return (0);
}

int f_vlist_prepack(const char *str)
{
    DEBUG(printf("f_vlist_prepack(\"%s\")\n", str));
    GLOBALS->vlist_prepack = atoi_64(str);
    return (0);
}

int f_wave_scrolling(const char *str)
{
    DEBUG(printf("f_wave_scrolling(\"%s\")\n", str));
    GLOBALS->wave_scrolling = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_zoom_base(const char *str)
{
    float f;
    DEBUG(printf("f_zoom_base(\"%s\")\n", str));
    sscanf(str, "%f", &f);
    if (f < 1.5)
        f = 1.5;
    else if (f > 10.0)
        f = 10.0;
    GLOBALS->zoombase = (gdouble)f;
    return (0);
}

int f_zoom_center(const char *str)
{
    DEBUG(printf("f_zoom_center(\"%s\")\n", str));
    GLOBALS->do_zoom_center = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_zoom_dynamic(const char *str)
{
    DEBUG(printf("f_zoom_dynamic(\"%s\")\n", str));
    GLOBALS->zoom_dyn = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_zoom_dynamic_end(const char *str)
{
    DEBUG(printf("f_zoom_dynamic_end(\"%s\")\n", str));
    GLOBALS->zoom_dyne = atoi_64(str) ? 1 : 0;
    return (0);
}

int f_zoom_pow10_snap(const char *str)
{
    DEBUG(printf("f_zoom_pow10_snap(\"%s\")\n", str));
    GLOBALS->zoom_pow10_snap = atoi_64(str) ? 1 : 0;
    return (0);
}

int rc_compare(const void *v1, const void *v2)
{
    return (strcasecmp((char *)v1, ((struct rc_entry *)v2)->name));
}

static void parse_color(const char *str, const char *property)
{
    GwColor color;
    if (gw_color_init_from_x11_name(&color, str) || gw_color_init_from_hex(&color, str)) {
        g_object_set(GLOBALS->color_theme, property, &color, NULL);

    } else {
#if defined __MINGW32__
        fprintf(stderr,
                "** gtkwave.ini (line %d): '%s' is an unknown color value; ignoring.\n",
                GLOBALS->rc_line_no,
                str);
#else
        fprintf(stderr,
                "** .gtkwaverc (line %d): '%s' is an unknown color value; ignoring.\n",
                GLOBALS->rc_line_no,
                str);
#endif
    }
}

/* make the color functions */
#define color_make(Z, prop) \
    int f_color_##Z(const char *str) \
    { \
        parse_color(str, prop); \
        return (0); \
    }

// clang-format off
color_make(back, "waveform-background")
color_make(baseline, "marker-baseline")
color_make(grid, "waveform-grid")
color_make(grid2, "waveform-grid2")
color_make(high, "stroke-h")
color_make(highfill, "fill-h")
color_make(low, "stroke-l")
color_make(1, "stroke-1")
color_make(1fill, "fill-1")
color_make(0, "stroke-0")
color_make(mark, "marker-named")
color_make(mid, "stroke-z")
color_make(trans, "stroke-transition")
color_make(umark, "marker-unnamed")
color_make(value, "waveform-value")
color_make(vbox, "vector-stroke")
//color_make(vtrans) TODO: remove
color_make(x, "stroke-x")
color_make(xfill, "fill-x")
color_make(u, "stroke-u")
color_make(ufill, "fill-u")
color_make(w, "stroke-w")
color_make(wfill, "fill-w")
color_make(dash, "stroke-dash")
color_make(dashfill, "fill-dash")

color_make(time, "timebar-text")
color_make(timeb, "timebar-background")

color_make(white , "signal-list-white")
color_make(black , "signal-list-black")
color_make(ltgray, "signal-list-ltgray")
color_make(normal, "signal-list-normal")
color_make(mdgray, "signal-list-mdgray")
color_make(dkgray, "signal-list-dkgray")
color_make(dkblue, "signal-list-dkblue")
color_make(brkred, "signal-list-brkred")
color_make(ltblue, "signal-list-ltblue")
color_make(gmstrd, "signal-list-gmstrd")
    // clang-format on

    // Unnecessary forward declaration to fix clang-format output.
    static struct rc_entry rcitems[];

/*
 * rc variables...these MUST be in alphabetical order for the bsearch!
 */
static struct rc_entry rcitems[] = {
    {"accel", f_accel},
    {"alt_hier_delimeter", f_alt_hier_delimeter},
    {"analog_redraw_skip_count", f_analog_redraw_skip_count},
    {"autocoalesce", f_autocoalesce},
    {"autocoalesce_reversal", f_autocoalesce_reversal},
    {"autoname_bundles", f_autoname_bundles},
    {"clipboard_mouseover", f_clipboard_mouseover},
    {"color_0", f_color_0},
    {"color_1", f_color_1},
    {"color_1fill", f_color_1fill},
    {"color_back", f_color_back},
    {"color_baseline", f_color_baseline},
    {"color_black", f_color_black},
    {"color_brkred", f_color_brkred},
    {"color_dash", f_color_dash},
    {"color_dashfill", f_color_dashfill},
    {"color_dkblue", f_color_dkblue},
    {"color_dkgray", f_color_dkgray},
    {"color_gmstrd", f_color_gmstrd},
    {"color_grid", f_color_grid},
    {"color_grid2", f_color_grid2},
    {"color_high", f_color_high},
    {"color_highfill", f_color_highfill},
    {"color_low", f_color_low},
    {"color_ltblue", f_color_ltblue},
    {"color_ltgray", f_color_ltgray},
    {"color_mark", f_color_mark},
    {"color_mdgray", f_color_mdgray},
    {"color_mid", f_color_mid},
    {"color_normal", f_color_normal},
    {"color_time", f_color_time},
    {"color_timeb", f_color_timeb},
    {"color_trans", f_color_trans},
    {"color_u", f_color_u},
    {"color_ufill", f_color_ufill},
    {"color_umark", f_color_umark},
    {"color_value", f_color_value},
    {"color_vbox", f_color_vbox},
    {"color_w", f_color_w},
    {"color_wfill", f_color_wfill},
    {"color_white", f_color_white},
    {"color_x", f_color_x},
    {"color_xfill", f_color_xfill},
    {"constant_marker_update", f_constant_marker_update},
    {"context_tabposition", f_context_tabposition},
    {"convert_to_reals", f_convert_to_reals},
    {"cursor_snap", f_cursor_snap},
    {"dbl_mant_dig_override", f_dbl_mant_dig_override},
    {"disable_antialiasing", f_disable_antialiasing},
    {"disable_auto_comphier", f_disable_auto_comphier},
    {"disable_empty_gui", f_disable_empty_gui},
    {"disable_mouseover", f_disable_mouseover},
    {"disable_tooltips", f_disable_tooltips},
    {"do_initial_zoom_fit", f_do_initial_zoom_fit},
    {"dragzoom_threshold", f_dragzoom_threshold},
    {"dynamic_resizing", f_dynamic_resizing},
    {"editor", f_editor},
    {"editor_run_in_terminal", f_editor_run_in_terminal},
    {"enable_fast_exit", f_enable_fast_exit},
    {"enable_ghost_marker", f_enable_ghost_marker},
    {"enable_horiz_grid", f_enable_horiz_grid},
    {"enable_vcd_autosave", f_enable_vcd_autosave},
    {"enable_vert_grid", f_enable_vert_grid},
    {"fill_waveform", f_fill_waveform},
    {"fontname_logfile", f_fontname_logfile},
    {"fontname_signals", f_fontname_signals},
    {"fontname_waves", f_fontname_waves},
    {"hier_delimeter", f_hier_delimeter},
    {"hier_ignore_escapes", f_hier_ignore_escapes},
    {"hier_max_level", f_hier_max_level},
    {"highlight_wavewindow", f_highlight_wavewindow},
    {"ignore_savefile_pane_pos", f_ignore_savefile_pane_pos},
    {"ignore_savefile_pos", f_ignore_savefile_pos},
    {"ignore_savefile_size", f_ignore_savefile_size},
    {"initial_signal_window_width", f_initial_signal_window_width},
    {"initial_window_x", f_initial_window_x},
    {"initial_window_xpos", f_initial_window_xpos},
    {"initial_window_y", f_initial_window_y},
    {"initial_window_ypos", f_initial_window_ypos},
    {"keep_xz_colors", f_keep_xz_colors},
    {"left_justify_sigs", f_left_justify_sigs},
    {"lz_removal", f_lz_removal},
    {"max_fsdb_trees", f_max_fsdb_trees},
    {"page_divisor", f_page_divisor},
    {"ps_maxveclen", f_ps_maxveclen},
    {"ruler_origin", f_ruler_origin},
    {"ruler_step", f_ruler_step},
    {"scale_to_time_dimension", f_scale_to_time_dimension},
    {"show_base_symbols", f_show_base_symbols},
    {"show_grid", f_show_grid},
    {"splash_disable", f_splash_disable},
    {"sst_dbl_action_type", f_sst_dbl_action_type},
    {"sst_dynamic_filter", f_sst_dynamic_filter},
    {"sst_expanded", f_sst_expanded},
    {"strace_repeat_count", f_strace_repeat_count},
    {"use_big_fonts", f_use_big_fonts},
    {"use_fat_lines", f_use_fat_lines},
    {"use_frequency_display", f_use_frequency_display},
    {"use_full_precision", f_use_full_precision},
    {"use_gestures", f_use_gestures},
    {"use_maxtime_display", f_use_maxtime_display},
    {"use_nonprop_fonts", f_use_nonprop_fonts},
    {"use_pango_fonts", f_use_pango_fonts},
    {"use_roundcaps", f_use_roundcaps},
    {"vcd_preserve_glitches", f_vcd_preserve_glitches},
    {"vcd_preserve_glitches_real", f_vcd_preserve_glitches_real},
    {"vcd_warning_filesize", f_vcd_warning_filesize},
    {"vector_padding", f_vector_padding},
    {"vlist_compression", f_vlist_compression},
    {"vlist_prepack", f_vlist_prepack},
    {"wave_scrolling", f_wave_scrolling},
    {"zoom_base", f_zoom_base},
    {"zoom_center", f_zoom_center},
    {"zoom_dynamic", f_zoom_dynamic},
    {"zoom_dynamic_end", f_zoom_dynamic_end},
    {"zoom_pow10_snap", f_zoom_pow10_snap}};

static void vanilla_rc(void)
{
    f_enable_fast_exit("on");
    f_splash_disable("off");
    f_zoom_pow10_snap("on");
    f_hier_max_level("1");
    f_cursor_snap("8");
    f_use_frequency_display("off");
    f_use_maxtime_display("off");
    f_use_roundcaps("on");
    f_use_nonprop_fonts("on");
    f_use_pango_fonts("on");
    f_constant_marker_update("on");
    f_show_base_symbols("off");
}

int insert_rc_variable(char *str)
{
    int i;
    int len;
    int ok = 0;

    len = strlen(str);
    if (len) {
        for (i = 0; i < len; i++) {
            int pos;
            if ((str[i] == ' ') || (str[i] == '\t'))
                continue; /* skip leading ws */
            if (str[i] == '#')
                break; /* is a comment */
            for (pos = i; i < len; i++) {
                if ((str[i] == ' ') || (str[i] == '\t')) {
                    str[i] = 0; /* null term envname */

                    for (i = i + 1; i < len; i++) {
                        struct rc_entry *r;

                        if ((str[i] == ' ') || (str[i] == '\t'))
                            continue;
                        if ((r = bsearch((void *)(str + pos),
                                         (void *)rcitems,
                                         sizeof(rcitems) / sizeof(struct rc_entry),
                                         sizeof(struct rc_entry),
                                         rc_compare))) {
                            int j;
                            for (j = len - 1; j >= i; j--) {
                                if ((str[j] == ' ') || (str[j] == '\t')) /* nuke trailing spaces */
                                {
                                    str[j] = 0;
                                    continue;
                                } else {
                                    break;
                                }
                            }
                            r->func(str + i); /* call resolution function */
                            ok = 1;
                        }
                        break;
                    }
                    break; /* added so multiple word values work properly*/
                }
            }
            break;
        }
    }

    return (ok);
}

void read_rc_file(const char *override_rc)
{
    FILE *handle = NULL;
    int i;
    int num_rcitems = sizeof(rcitems) / sizeof(struct rc_entry);
    gboolean good_override = FALSE;

    for (i = 0; i < (num_rcitems - 1); i++) {
        if (strcmp(rcitems[i].name, rcitems[i + 1].name) > 0) {
            fprintf(stderr,
                    "rcitems misordering: '%s' vs '%s'\n",
                    rcitems[i].name,
                    rcitems[i + 1].name);
            exit(255);
        }
    }

    /* move defaults first and only go whitescreen if instructed to do so */
    if (GLOBALS->possibly_use_rc_defaults)
        vanilla_rc();

    if ((override_rc) && ((handle = fopen(override_rc, "rb")))) {
        /* good, we have a handle */
        wave_gconf_client_set_string("/current/rcfile", override_rc);
        good_override = TRUE;
    } else {
        if (override_rc) {
            fprintf(stderr, "GTKWAVE | rcfile '%s' not found, attempting defaults.\n", override_rc);
        }
    }

    if (good_override) {
        /* nothing, have file handle */
    } else
#if !defined __MINGW32__
        if (!(handle = fopen(rcname, "rb"))) {
        struct passwd *pw = NULL;
        char *home = NULL;
        char *rcpath = NULL;

        pw = getpwuid(geteuid());
        if (pw) {
            home = pw->pw_dir;
        }

        if (!home) {
            home = getenv("HOME");
        }

        if (home) {
            rcpath = (char *)alloca(strlen(home) + 1 + strlen(rcname) + 1);
            strcpy(rcpath, home);
            strcat(rcpath, "/");
            strcat(rcpath, rcname);
        }

        if (!rcpath || !(handle = fopen(rcpath, "rb"))) {
#ifdef MAC_INTEGRATION
            const gchar *bundle_id = gtkosx_application_get_bundle_id();
            if (bundle_id) {
                const gchar *rpath = gtkosx_application_get_resource_path();
                const char *suf = "/gtkwaverc";

                rcpath = NULL;
                if (rpath) {
                    rcpath = (char *)alloca(strlen(rpath) + strlen(suf) + 1);
                    strcpy(rcpath, rpath);
                    strcat(rcpath, suf);
                }

                if (!rcpath || !(handle = fopen(rcpath, "rb"))) {
                    wave_gconf_client_set_string("/current/rcfile", "");
                    errno = 0;
                    return; /* no .rc file */
                } else {
                    wave_gconf_client_set_string("/current/rcfile", rcpath);
                }
            } else
#endif
            {
                wave_gconf_client_set_string("/current/rcfile", "");
                errno = 0;
                return; /* no .rc file */
            }
        } else {
            wave_gconf_client_set_string("/current/rcfile", rcpath);
        }
    }
#else
        if (!(handle = fopen(rcname, "rb"))) /* no concept of ~ in win32 */
    {
        /* Try to find rcname in USERPROFILE */
        char *home;
        char *rcpath;

        home = getenv("USERPROFILE");
        if (home != NULL) {
            /* printf("USERPROFILE = %s\n", home); */
            rcpath = (char *)alloca(strlen(home) + 1 + strlen(rcname) + 1);
            strcpy(rcpath, home);
            strcat(rcpath, "\\");
            strcat(rcpath, rcname);
            /* printf("rcpath = %s\n", rcpath); */
        }
        if ((home == NULL) || (!(handle = fopen(rcpath, "rb")))) {
            /* printf("No rc file\n"); */
            wave_gconf_client_set_string("/current/rcfile", "");
            errno = 0;
            if (GLOBALS->possibly_use_rc_defaults)
                vanilla_rc();
            return; /* no .rc file */
        }

        wave_gconf_client_set_string("/current/rcfile", rcpath);
    }
#endif

    GLOBALS->rc_line_no = 0;
    while (!feof(handle)) {
        char *str;

        GLOBALS->rc_line_no++;
        if ((str = fgetmalloc(handle))) {
            insert_rc_variable(str);
            free_2(str);
        }
    }

    fclose(handle);
    errno = 0;
    return;
}

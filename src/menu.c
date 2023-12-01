/*
 * Copyright (c) Tony Bybell 1999-2017.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

/*
 * note: any functions which add/remove traces must first look at
 * the global "straces".  if it's active, complain to the status
 * window and don't do the op. same for "dnd_state".
 */

#include "globals.h"
#include <config.h>
#include <string.h>
#include "gtk23compat.h"
#include "main.h"
#include "menu.h"
#include "vcd.h"
#include "vcd_saver.h"
#include "translate.h"
#include "ptranslate.h"
#include "ttranslate.h"
#include "lx2.h"
#include "hierpack.h"
#include "tcl_helper.h"
#include "signal_list.h"
#include "gw-named-marker-dialog.h"
#include <cocoa_misc.h>
#include <assert.h>

#if !defined __MINGW32__
#include <unistd.h>
#include <sys/mman.h>
#else
#include <windows.h>
#include <io.h>
#endif

#undef WAVE_USE_MENU_BLACKOUTS

static gtkwave_mlist_t menu_items[WV_MENU_NUMITEMS];
static GtkWidget **menu_wlist = NULL;

extern char *gtkwave_argv0_cached; /* for new window */

/* marshals for handling menu items vs button pressed items */

static void service_left_edge_marshal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)callback_action;

    service_left_edge(widget, null_data);
}

static void service_right_edge_marshal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)callback_action;

    service_right_edge(widget, null_data);
}

static void service_zoom_in_marshal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)callback_action;

    service_zoom_in(widget, null_data);
}

static void service_zoom_out_marshal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)callback_action;

    service_zoom_out(widget, null_data);
}

static void service_zoom_full_marshal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)callback_action;

    service_zoom_full(widget, null_data);
}

static void service_zoom_fit_marshal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)callback_action;

    service_zoom_fit(widget, null_data);
}

static void service_zoom_left_marshal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)callback_action;

    service_zoom_left(widget, null_data);
}

static void service_zoom_right_marshal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)callback_action;

    service_zoom_right(widget, null_data);
}

static void service_zoom_undo_marshal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)callback_action;

    service_zoom_undo(widget, null_data);
}

static void fetch_right_marshal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)callback_action;

    fetch_right(widget, null_data);
}

static void fetch_left_marshal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)callback_action;

    fetch_left(widget, null_data);
}

static void discard_right_marshal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)callback_action;

    discard_right(widget, null_data);
}

static void discard_left_marshal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)callback_action;

    discard_left(widget, null_data);
}

static void service_right_shift_marshal(gpointer null_data,
                                        guint callback_action,
                                        GtkWidget *widget)
{
    (void)callback_action;

    service_right_shift(widget, null_data);
}

static void service_left_shift_marshal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)callback_action;

    service_left_shift(widget, null_data);
}

static void service_right_page_marshal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)callback_action;

    service_right_page(widget, null_data);
}

static void service_left_page_marshal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)callback_action;

    service_left_page(widget, null_data);
}

/* ruler */

static void menu_def_ruler(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
    GwMarker *baseline_marker = gw_project_get_baseline_marker(GLOBALS->project);

    if (gw_marker_is_enabled(baseline_marker) && gw_marker_is_enabled(primary_marker)) {
        GLOBALS->ruler_origin = gw_marker_get_position(baseline_marker);
        GLOBALS->ruler_step =
            ABS(gw_marker_get_position(baseline_marker) - gw_marker_get_position(primary_marker));
    } else {
        GLOBALS->ruler_origin = 0;
        GLOBALS->ruler_step = 0;
    }

    GLOBALS->signalwindow_width_dirty = 1;
    redraw_signals_and_waves();
}

/* marker locking */

static void lock_marker_left(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    g_error("lock_marker_left disabled");

    // int ent_idx = GLOBALS->named_marker_lock_idx;
    // int i;
    // int success = 0;

    // if (ent_idx < 0)
    //     ent_idx = WAVE_NUM_NAMED_MARKERS;
    // ent_idx--;
    // if (ent_idx < 0)
    //     ent_idx = WAVE_NUM_NAMED_MARKERS - 1;

    // for (i = 0; i < WAVE_NUM_NAMED_MARKERS; i++) {
    //     if (GLOBALS->named_markers[ent_idx] >= 0) {
    //         success = 1;
    //         break;
    //     }

    //     ent_idx--;
    //     if (ent_idx < 0)
    //         ent_idx = WAVE_NUM_NAMED_MARKERS - 1;
    // }

    // if (!success) {
    //     ent_idx = 0;
    //     GLOBALS->named_markers[ent_idx] = GLOBALS->tims.marker;
    // }

    // GLOBALS->named_marker_lock_idx = ent_idx;
    // GLOBALS->tims.marker = GLOBALS->named_markers[ent_idx];

    // update_time_box();
    // GLOBALS->signalwindow_width_dirty = 1;
    // redraw_signals_and_waves();
}

static void lock_marker_right(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    g_error("lock_marker_right disabled");

    // int ent_idx = GLOBALS->named_marker_lock_idx;
    // int i;
    // int success = 0;

    // if (ent_idx < 0)
    //     ent_idx = -1; /* not really necessary */
    // ent_idx++;
    // if (ent_idx > (WAVE_NUM_NAMED_MARKERS - 1))
    //     ent_idx = 0;

    // for (i = 0; i < WAVE_NUM_NAMED_MARKERS; i++) {
    //     if (GLOBALS->named_markers[ent_idx] >= 0) {
    //         success = 1;
    //         break;
    //     }

    //     ent_idx++;
    //     if (ent_idx > (WAVE_NUM_NAMED_MARKERS - 1))
    //         ent_idx = 0;
    // }

    // if (!success) {
    //     ent_idx = 0;
    //     GLOBALS->named_markers[ent_idx] = GLOBALS->tims.marker;
    // }

    // GLOBALS->named_marker_lock_idx = ent_idx;
    // GLOBALS->tims.marker = GLOBALS->named_markers[ent_idx];

    // update_time_box();

    // GLOBALS->signalwindow_width_dirty = 1;
    // redraw_signals_and_waves();
}

static void unlock_marker(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GLOBALS->named_marker_lock_idx = -1;

    GLOBALS->signalwindow_width_dirty = 1;
    redraw_signals_and_waves();
}

/* toggles for time dimension conversion */

static void menu_scale_to_td_common(GtkCheckMenuItem *menu_item, char dimension)
{
    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_TDSCALEX]), TRUE);
    } else {
        if (gtk_check_menu_item_get_active(menu_item)) {
            GLOBALS->scale_to_time_dimension = dimension;
            set_scale_to_time_dimension_toggles();
            redraw_signals_and_waves();
        }
    }
}

void menu_scale_to_td_x(gpointer data, guint callback_action, GtkWidget *widget)
{
    menu_scale_to_td_common(GTK_CHECK_MENU_ITEM(data), 0);
}

void menu_scale_to_td_s(gpointer data, guint callback_action, GtkWidget *widget)
{
    menu_scale_to_td_common(GTK_CHECK_MENU_ITEM(data), 's');
}

void menu_scale_to_td_m(gpointer data, guint callback_action, GtkWidget *widget)
{
    menu_scale_to_td_common(GTK_CHECK_MENU_ITEM(data), 'm');
}

void menu_scale_to_td_u(gpointer data, guint callback_action, GtkWidget *widget)
{
    menu_scale_to_td_common(GTK_CHECK_MENU_ITEM(data), 'u');
}

void menu_scale_to_td_n(gpointer data, guint callback_action, GtkWidget *widget)
{
    menu_scale_to_td_common(GTK_CHECK_MENU_ITEM(data), 'n');
}

void menu_scale_to_td_p(gpointer data, guint callback_action, GtkWidget *widget)
{
    menu_scale_to_td_common(GTK_CHECK_MENU_ITEM(data), 'p');
}

void menu_scale_to_td_f(gpointer data, guint callback_action, GtkWidget *widget)
{
    menu_scale_to_td_common(GTK_CHECK_MENU_ITEM(data), 'f');
}

/********** transaction procsel filter install ********/

void menu_dataformat_xlate_ttrans_1(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    ttrans_searchbox("Select Transaction Filter Process");
}

void menu_dataformat_xlate_ttrans_0(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    install_ttrans_filter(0); /* disable, 0 is always NULL */
}

/********** procsel filter install ********/

void menu_dataformat_xlate_proc_1(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    ptrans_searchbox("Select Signal Filter Process");
}

void menu_dataformat_xlate_proc_0(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    install_proc_filter(0); /* disable, 0 is always NULL */
}

/********** filesel filter install ********/

void menu_dataformat_xlate_file_1(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    trans_searchbox("Select Signal Filter");
}

void menu_dataformat_xlate_file_0(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    install_file_filter(0); /* disable, 0 is always NULL */
}

/******************************************************************/

void menu_write_vcd_file_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    int rc;

    if (!GLOBALS->filesel_ok) {
        return;
    }

    if (GLOBALS->lock_menu_c_2 == 1)
        return; /* avoid recursion */
    GLOBALS->lock_menu_c_2 = 1;

    status_text("Saving VCD...\n");
    gtkwave_main_iteration(); /* make requester disappear requester */

    rc = save_nodes_to_export(*GLOBALS->fileselbox_text, WAVE_EXPORT_VCD);

    GLOBALS->lock_menu_c_2 = 0;

    switch (rc) {
        case VCDSAV_EMPTY:
            status_text("No traces onscreen to save!\n");
            break;

        case VCDSAV_FILE_ERROR:
            status_text("Problem writing VCD: ");
            status_text(strerror(errno));
            break;

        case VCDSAV_OK:
            status_text("VCD written successfully.\n");
        default:
            break;
    }
}

void menu_write_vcd_file(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->traces.first) {
        fileselbox("Write VCD File As",
                   &GLOBALS->filesel_vcd_writesave,
                   G_CALLBACK(menu_write_vcd_file_cleanup),
                   G_CALLBACK(NULL),
                   "*.vcd",
                   1);
    } else {
        status_text("No traces onscreen to save!\n");
    }
}

/******************************************************************/

void menu_write_tim_file_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    int rc;

    if (!GLOBALS->filesel_ok) {
        return;
    }

    if (GLOBALS->lock_menu_c_2 == 1)
        return; /* avoid recursion */
    GLOBALS->lock_menu_c_2 = 1;

    status_text("Saving TIM...\n");
    gtkwave_main_iteration(); /* make requester disappear requester */

    rc = save_nodes_to_export(*GLOBALS->fileselbox_text, WAVE_EXPORT_TIM);

    GLOBALS->lock_menu_c_2 = 0;

    switch (rc) {
        case VCDSAV_EMPTY:
            status_text("No traces onscreen to save!\n");
            break;

        case VCDSAV_FILE_ERROR:
            status_text("Problem writing TIM: ");
            status_text(strerror(errno));
            break;

        case VCDSAV_OK:
            status_text("TIM written successfully.\n");
        default:
            break;
    }
}

void menu_write_tim_file(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->traces.first) {
        fileselbox("Write TIM File As",
                   &GLOBALS->filesel_tim_writesave,
                   G_CALLBACK(menu_write_tim_file_cleanup),
                   G_CALLBACK(NULL),
                   "*.tim",
                   1);
    } else {
        status_text("No traces onscreen to save!\n");
    }
}

/******************************************************************/

void menu_unwarp_traces_all(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwTrace *t;
    int found = 0;

    t = GLOBALS->traces.first;
    while (t) {
        if (t->shift) {
            t->shift = GW_TIME_CONSTANT(0);
            found++;
        }
        t = t->t_next;
    }

    if (found) {
        GLOBALS->signalwindow_width_dirty = 1;
        redraw_signals_and_waves();
    }
}

void menu_unwarp_traces(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwTrace *t;
    int found = 0;

    t = GLOBALS->traces.first;
    while (t) {
        if (t->flags & TR_HIGHLIGHT) {
            t->shift = GW_TIME_CONSTANT(0);
            t->flags &= (~TR_HIGHLIGHT);
            found++;
        }
        t = t->t_next;
    }

    if (found) {
        GLOBALS->signalwindow_width_dirty = 1;
        redraw_signals_and_waves();
    }
}

void warp_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    if (GLOBALS->entrybox_text) {
        GwTime gt, delta;
        GwTrace *t;

        gt = unformat_time(GLOBALS->entrybox_text, GLOBALS->time_dimension);
        free_2(GLOBALS->entrybox_text);
        GLOBALS->entrybox_text = NULL;

        if (gt < 0) {
            delta = GLOBALS->tims.first - GLOBALS->tims.last;
            if (gt < delta)
                gt = delta;
        } else if (gt > 0) {
            delta = GLOBALS->tims.last - GLOBALS->tims.first;
            if (gt > delta)
                gt = delta;
        }

        t = GLOBALS->traces.first;
        while (t) {
            if (t->flags & TR_HIGHLIGHT) {
                if (HasWave(t)) /* though note if a user specifies comment warping in a .sav file we
                                   will honor it.. */
                {
                    t->shift = gt;
                } else {
                    t->shift = GW_TIME_CONSTANT(0);
                }
                t->flags &= (~TR_HIGHLIGHT);
            }
            t = t->t_next;
        }
    }

    GLOBALS->signalwindow_width_dirty = 1;
    redraw_signals_and_waves();
}

void menu_warp_traces(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    char gt[32];
    GwTrace *t;
    int found = 0;

    t = GLOBALS->traces.first;
    while (t) {
        if (t->flags & TR_HIGHLIGHT) {
            found++;
            break;
        }
        t = t->t_next;
    }

    if (found) {
        reformat_time(gt, GW_TIME_CONSTANT(0), GLOBALS->time_dimension);
        entrybox("Warp Traces", 200, gt, NULL, 20, G_CALLBACK(warp_cleanup));
    }
}

void wave_scrolling_on(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_MWSON]),
            GLOBALS->wave_scrolling =
                GLOBALS->wave_script_args
                    ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                    : (!GLOBALS->wave_scrolling));
    } else {
        GLOBALS->wave_scrolling =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_MWSON]));
        if (GLOBALS->wave_scrolling) {
            status_text("Wave Scrolling On.\n");
        } else {
            status_text("Wave Scrolling Off.\n");
        }
    }
}

/**/

void menu_keep_xz_colors(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_KEEPXZ]),
            GLOBALS->keep_xz_colors =
                GLOBALS->wave_script_args
                    ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                    : (!GLOBALS->keep_xz_colors));
    } else {
        GLOBALS->keep_xz_colors =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_KEEPXZ]));
    }

    GLOBALS->signalwindow_width_dirty = 1;
    redraw_signals_and_waves();
}

/**/

void menu_autocoalesce(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_ACOL]),
            GLOBALS->autocoalesce =
                GLOBALS->wave_script_args
                    ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                    : (!GLOBALS->autocoalesce));
    } else {
        GLOBALS->autocoalesce =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_ACOL]));
        if (GLOBALS->autocoalesce) {
            status_text("Autocoalesce On.\n");
        } else {
            status_text("Autocoalesce Off.\n");
        }
    }
}

void menu_autocoalesce_reversal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_ACOLR]),
            GLOBALS->autocoalesce_reversal =
                GLOBALS->wave_script_args
                    ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                    : (!GLOBALS->autocoalesce_reversal));
    } else {
        GLOBALS->autocoalesce_reversal =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_ACOLR]));
        if (GLOBALS->autocoalesce_reversal) {
            status_text("Autocoalesce Rvs On.\n");
        } else {
            status_text("Autocoalesce Rvs Off.\n");
        }
    }
}

void menu_autoname_bundles_on(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_ABON]),
            GLOBALS->autoname_bundles =
                GLOBALS->wave_script_args
                    ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                    : (!GLOBALS->autoname_bundles));
    } else {
        GLOBALS->autoname_bundles =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_ABON]));
        if (GLOBALS->autoname_bundles) {
            status_text("Autoname On.\n");
        } else {
            status_text("Autoname Off.\n");
        }
    }
}

void set_hier_cleanup(GtkWidget *widget, gpointer data, int level)
{
    (void)widget;
    (void)data;

    char update_string[128];
    GwTrace *t;
    int i;

    GLOBALS->hier_max_level = level;
    if (GLOBALS->hier_max_level < 0)
        GLOBALS->hier_max_level = 0;

    for (i = 0; i < 2; i++) {
        if (i == 0)
            t = GLOBALS->traces.first;
        else
            t = GLOBALS->traces.buffer;

        while (t) {
            if (!(t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH))) {
                if (HasAlias(t)) {
                    t->name = t->name_full;
                    if (GLOBALS->hier_max_level)
                        t->name = hier_extract(t->name, GLOBALS->hier_max_level);
                } else if (t->vector == TRUE) {
                    t->name = t->n.vec->bvname;
                    if (GLOBALS->hier_max_level)
                        t->name = hier_extract(t->name, GLOBALS->hier_max_level);
                } else {
                    if (!GLOBALS->hier_max_level) {
                        int flagged = HIER_DEPACK_ALLOC;

                        if (t->name && t->is_depacked) {
                            free_2(t->name);
                        }
                        t->name = hier_decompress_flagged(t->n.nd->nname, &flagged);
                        t->is_depacked = (flagged != 0);
                    } else {
                        int flagged = HIER_DEPACK_ALLOC;
                        char *tbuff;

                        if (t->name && t->is_depacked) {
                            free_2(t->name);
                        }
                        tbuff = hier_decompress_flagged(t->n.nd->nname, &flagged);
                        t->is_depacked = (flagged != 0);

                        if (!flagged) {
                            t->name = hier_extract(t->n.nd->nname, GLOBALS->hier_max_level);
                        } else {
                            t->name = strdup_2(hier_extract(tbuff, GLOBALS->hier_max_level));
                            free_2(tbuff);
                        }
                    }
                }
            }
            t = t->t_next;
        }
    }

    GLOBALS->signalwindow_width_dirty = 1;
    redraw_signals_and_waves();
    sprintf(update_string, "Trace Hier Max Depth is now: %d\n", GLOBALS->hier_max_level);
    status_text(update_string);
}

void max_hier_cleanup(GtkWidget *widget, gpointer data)
{
    if (GLOBALS->entrybox_text) {
        int i;

        i = atoi_64(GLOBALS->entrybox_text);
        set_hier_cleanup(widget, data, i);
        GLOBALS->hier_max_level_shadow = GLOBALS->hier_max_level; /* used for the toggle function */

        free_2(GLOBALS->entrybox_text);
        GLOBALS->entrybox_text = NULL;
    }
}

void menu_set_max_hier(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    char za[32];

    sprintf(za, "%d", GLOBALS->hier_max_level);

    entrybox("Max Hier Depth", 200, za, NULL, 20, G_CALLBACK(max_hier_cleanup));
}

void menu_toggle_hier(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)callback_action;

    if (GLOBALS->hier_max_level)
        set_hier_cleanup(widget, null_data, 0);
    else
        set_hier_cleanup(widget,
                         null_data,
                         GLOBALS->hier_max_level_shadow); /* instead of just '1' */
}

/**/
void menu_use_roundcaps(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VDRV]),
            GLOBALS->use_roundcaps =
                GLOBALS->wave_script_args
                    ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                    : (!GLOBALS->use_roundcaps));
    } else {
        GLOBALS->use_roundcaps =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VDRV]));
        if (GLOBALS->use_roundcaps) {
            status_text("Using roundcaps.\n");
        } else {
            status_text("Using flatcaps.\n");
        }
        redraw_signals_and_waves();
    }
}

/**/
void menu_use_full_precision(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VFTP]),
            GLOBALS->use_full_precision =
                GLOBALS->wave_script_args
                    ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                    : (!GLOBALS->use_full_precision));
    } else {
        GLOBALS->use_full_precision =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VFTP]));
        if (!GLOBALS->use_full_precision) {
            status_text("Full Prec Off.\n");
        } else {
            status_text("Full Prec On.\n");
        }

        calczoom(GLOBALS->tims.zoom);

        if (GLOBALS->wave_hslider) {
            fix_wavehadj();

            g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                  "changed"); /* force zoom update */
            g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                  "value_changed"); /* force zoom update */

            update_time_box();
        }
    }
}
/**/
void menu_remove_marked(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    int i;

    WAVE_STRACE_ITERATOR(i)
    {
        GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = i];

        if (GLOBALS->strace_ctx->shadow_straces) {
            delete_strace_context();
        }

        strace_maketimetrace(0);
    }

    redraw_signals_and_waves();
}
/**/
void menu_use_color(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GLOBALS->black_and_white = 0;
    redraw_signals_and_waves();
}
/**/
void menu_use_bw(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GLOBALS->black_and_white = 1;
    redraw_signals_and_waves();
}
/**/
void menu_zoom10_snap(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VZPS]),
            GLOBALS->zoom_pow10_snap =
                GLOBALS->wave_script_args
                    ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                    : (!GLOBALS->zoom_pow10_snap));
    } else {
        GLOBALS->zoom_pow10_snap =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VZPS]));
        if (!GLOBALS->zoom_pow10_snap) {
            status_text("Pow10 Snap Off.\n");
        } else {
            status_text("Pow10 Snap On.\n");
        }

        if (GLOBALS->wave_hslider) {
            calczoom(GLOBALS->tims.zoom);
            fix_wavehadj();

            g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                  "changed"); /* force zoom update */
            g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                  "value_changed"); /* force zoom update */
        }
    }
}

/**/
void menu_zoom_dynf(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        if (menu_wlist[WV_MENU_VZDYN]) /* not always available */
        {
            gtk_check_menu_item_set_active(
                GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VZDYN]),
                GLOBALS->zoom_dyn =
                    GLOBALS->wave_script_args
                        ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                        : (!GLOBALS->zoom_dyn));
        }
    } else {
        GLOBALS->zoom_dyn =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VZDYN]));
        if (!GLOBALS->zoom_dyn) {
            status_text("Dynamic Zoom Full Off.\n");
        } else {
            status_text("Dynamic Zoom Full On.\n");
        }
    }
}

/**/
void menu_zoom_dyne(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        if (menu_wlist[WV_MENU_VZDYNE]) /* not always available */
        {
            gtk_check_menu_item_set_active(
                GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VZDYNE]),
                GLOBALS->zoom_dyne =
                    GLOBALS->wave_script_args
                        ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                        : (!GLOBALS->zoom_dyne));
        }
    } else {
        GLOBALS->zoom_dyne =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VZDYNE]));
        if (!GLOBALS->zoom_dyne) {
            status_text("Dynamic Zoom To End Off.\n");
        } else {
            status_text("Dynamic Zoom To End On.\n");
        }
    }
}

/**/
void menu_left_justify(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    status_text("Left Justification.\n");
    GLOBALS->left_justify_sigs = ~0;
    MaxSignalLength();
    gw_signal_list_force_redraw(GW_SIGNAL_LIST(GLOBALS->signalarea));
}

/**/
void menu_right_justify(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    status_text("Right Justification.\n");
    GLOBALS->left_justify_sigs = 0;
    MaxSignalLength();
    gw_signal_list_force_redraw(GW_SIGNAL_LIST(GLOBALS->signalarea));
}

/**/
void menu_enable_constant_marker_update(gpointer null_data,
                                        guint callback_action,
                                        GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VCMU]),
            GLOBALS->constant_marker_update =
                GLOBALS->wave_script_args
                    ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                    : (!GLOBALS->constant_marker_update));
    } else {
        GLOBALS->constant_marker_update =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VCMU]));
        if (GLOBALS->constant_marker_update) {
            status_text("Constant marker update enabled.\n");
        } else {
            status_text("Constant marker update disabled.\n");
        }
    }
}
/**/
void menu_enable_dynamic_resize(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;
    int i;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VDR]),
            GLOBALS->do_resize_signals =
                GLOBALS->wave_script_args
                    ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                    : (!GLOBALS->do_resize_signals));
    } else {
        GLOBALS->do_resize_signals =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VDR]));
        if (GLOBALS->do_resize_signals) {
            status_text("Resizing enabled.\n");
        } else {
            status_text("Resizing disabled.\n");
        }

        if (GLOBALS->signalarea && GLOBALS->wavearea) {
            for (i = 0; i < 2; i++) {
                GLOBALS->signalwindow_width_dirty = 1;
                redraw_signals_and_waves();
            }
        }
    }
}
/**/
void menu_toggle_delta_or_frequency(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GLOBALS->use_frequency_delta = (GLOBALS->use_frequency_delta) ? 0 : 1;
    update_time_box();
}
/**/
void menu_toggle_max_or_marker(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GLOBALS->use_maxtime_display = (GLOBALS->use_maxtime_display) ? 0 : 1;
    update_time_box();
}
/**/
#ifdef MAC_INTEGRATION
void menu_help_manual(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    const gchar *bundle_id = gtkosx_application_get_bundle_id();
    if (bundle_id) {
        const gchar *rpath = gtkosx_application_get_resource_path();
        const char *suf = "/doc/gtkwave.pdf";
        char *pdfpath = NULL;
        FILE *handle;

        if (rpath) {
            pdfpath = (char *)alloca(strlen(rpath) + strlen(suf) + 1);
            strcpy(pdfpath, rpath);
            strcat(pdfpath, suf);
        }

        if (!pdfpath || !(handle = fopen(pdfpath, "rb"))) {
        } else {
            fclose(handle);
            gtk_open_external_file(pdfpath);
            return;
        }
    }

    simplereqbox("Wave User's Guide", 400, "Could not open PDF!", "OK", NULL, NULL, 1);
}
#endif
/**/
void menu_version(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    static const gchar *AUTHORS[] = {
        "Tony Bybell",
        "Udi Finkelstein",
        "Kermin Elliott Fleming",
        "Donald Baltus",
        "Tristan Gingold",
        "Thomas Sailer",
        "Concept Engineering GmbH",
        "Ralf Fuest",
        NULL,
    };

    // clang-format off
    gtk_show_about_dialog(NULL,
                          "program-name", "GTKWave",
                          "logo-icon-name", "io.github.gtkwave.GTKWave",
                          "version", PACKAGE_VERSION,
                          "copyright", "Copyright 1999-2023 BSI",
                          "authors", AUTHORS,
                          "license-type", GTK_LICENSE_GPL_2_0,
                          "website", "https://gtkwave.sourceforge.net",
                          NULL);
    // clang-format on
}
/**/
void menu_quit_callback(GtkWidget *widget, gpointer data)
{
    (void)widget;

    char sstr[32];

    if (data) {
#ifdef __CYGWIN__
        kill_stems_browser();
#endif
        g_print("Exiting.\n");
        sprintf(sstr, "%d", GLOBALS->this_context_page);
        gtkwavetcl_setvar(WAVE_TCLCB_QUIT_PROGRAM, sstr, WAVE_TCLCB_QUIT_PROGRAM_FLAGS);

        exit(0);
    }
}
void menu_quit(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->save_on_exit) {
        menu_write_save_file(NULL, 0, NULL);
    }

    if (!GLOBALS->enable_fast_exit) {
        simplereqbox("Quit Program",
                     300,
                     "Do you really want to quit?",
                     "Yes",
                     "No",
                     G_CALLBACK(menu_quit_callback),
                     1);
    } else {
        menu_quit_callback(NULL, &GLOBALS->enable_fast_exit); /* nonzero dummy arg */
    }
}

/**/

void menu_quit_close_callback(GtkWidget *widget, gpointer dummy_data)
{
    (void)widget;
    (void)dummy_data;

    unsigned int i, j = 0;
    unsigned int this_page = GLOBALS->this_context_page;
    unsigned np = GLOBALS->num_notebook_pages;
    unsigned int new_page = (this_page != np - 1) ? this_page : (this_page - 1);
    GtkWidget *n = GLOBALS->notebook;
    struct Global *old_g = NULL, *saved_g;
    char sstr[32];
    gboolean is_mf = (GLOBALS->loaded_file_type == MISSING_FILE);

    sprintf(sstr, "%d", this_page);
    gtkwavetcl_setvar(WAVE_TCLCB_CLOSE_TAB_NUMBER, sstr, WAVE_TCLCB_CLOSE_TAB_NUMBER_FLAGS);

    kill_stems_browser_single(GLOBALS);
    dead_context_sweep();

    for (i = 0; i < np; i++) {
        if (i != this_page) {
            (*GLOBALS->contexts)[j] = (*GLOBALS->contexts)[i];
            (*GLOBALS->contexts)[j]->this_context_page = j;
            (*GLOBALS->contexts)[j]->num_notebook_pages--;

            j++;
        } else {
            old_g = (*GLOBALS->contexts)[j];
        }
    }
    (*GLOBALS->contexts)[j] = old_g;

    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(n), (np > 2));
    gtk_notebook_set_show_border(GTK_NOTEBOOK(n), (np > 2));

    gtk_notebook_remove_page(GTK_NOTEBOOK(n), this_page);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(n), new_page);

    set_GLOBALS((*GLOBALS->contexts)[new_page]);
    saved_g = GLOBALS;

    gtkwave_main_iteration();

    set_GLOBALS(old_g);
    if (!is_mf) {
        free_and_destroy_page_context();
    }
    set_GLOBALS(saved_g);

    /* need to do this if 2 pages -> 1 */
    reformat_time(sstr, GLOBALS->tims.first, GLOBALS->time_dimension);
    gtk_entry_set_text(GTK_ENTRY(GLOBALS->from_entry), sstr);
    reformat_time(sstr, GLOBALS->tims.last, GLOBALS->time_dimension);
    gtk_entry_set_text(GTK_ENTRY(GLOBALS->to_entry), sstr);
    update_time_box();

    gtk_window_set_title(GTK_WINDOW(GLOBALS->mainwindow), GLOBALS->winname);

    redraw_signals_and_waves();
}

void menu_quit_close(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if ((GLOBALS->num_notebook_pages < 2) && (!GLOBALS->enable_fast_exit)) {
        simplereqbox("Quit Program",
                     300,
                     "Do you really want to quit?",
                     "Yes",
                     "No",
                     G_CALLBACK(menu_quit_callback),
                     1);
    } else {
        if (GLOBALS->num_notebook_pages < 2) {
            menu_quit_callback(NULL, &GLOBALS->num_notebook_pages); /* nonzero dummy arg */
        } else {
            menu_quit_close_callback(NULL, NULL); /* dummy arg, not needed to be nonzero */
        }
    }
}

/**/
void must_sel(void)
{
    status_text("Select one or more traces.\n");
}
static void must_sel_nb(void)
{
    status_text("Select one or more nonblank traces.\n");
}
static void must_sel_bg(void)
{
    status_text("Select a bundle or group.\n");
}
/**/

static void menu_open_group(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    GwTrace *t;
    unsigned dirty = 0;

    /* currently only called by toggle menu option, so no help menu text */

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    t = GLOBALS->traces.first;
    while (t) {
        if ((t->flags & TR_HIGHLIGHT) && (IsGroupBegin(t) || IsGroupEnd(t))) {
            dirty = 1;
            break;
        }
        t = t->t_next;
    }

    if (dirty) {
        OpenTrace(t);
        GLOBALS->signalwindow_width_dirty = 1;
        redraw_signals_and_waves();
    } else {
        must_sel_bg();
    }
}

static void menu_close_group(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    GwTrace *t;
    unsigned dirty = 0;

    /* currently only called by toggle menu option, so no help menu text */

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    t = GLOBALS->traces.first;
    while (t) {
        if ((t->flags & TR_HIGHLIGHT) && (IsGroupBegin(t) || IsGroupEnd(t))) {
            dirty = 1;
            break;
        }
        t = t->t_next;
    }

    if (dirty) {
        CloseTrace(t);
        GLOBALS->signalwindow_width_dirty = 1;
        redraw_signals_and_waves();
    } else {
        must_sel_bg();
    }
}

unsigned create_group(const char *name, GwTrace *t_composite)
{
    GwTrace *t;
    GwTrace *t_prev;
    GwTrace *t_begin;
    GwTrace *t_end;
    unsigned dirty = 0;

    if (!name)
        name = "Group"; /* generate anonymous name */

    t = GLOBALS->traces.first;
    while (t) {
        if (t->flags & TR_HIGHLIGHT) {
            dirty = 1;
            break;
        }
        t = t->t_next;
    }

    if (dirty) {
        t_prev = t->t_prev;

        CutBuffer();

        if (t_composite) {
            t_begin = t_composite;
            t_begin->flags |= TR_GRP_BEGIN;
        } else {
            if ((t_begin = calloc_2(1, sizeof(GwTrace))) == NULL) {
                fprintf(stderr, "Out of memory, can't add trace.\n");
                return (0);
            }

            t_begin->flags = (TR_BLANK | TR_GRP_BEGIN);
            t_begin->name = (char *)malloc_2(1 + strlen(name));
            strcpy(t_begin->name, name);
        }

        GLOBALS->traces.buffer->t_prev = t_begin;
        t_begin->t_next = GLOBALS->traces.buffer;
        GLOBALS->traces.buffer = t_begin;
        GLOBALS->traces.buffercount++;

        if ((t_end = calloc_2(1, sizeof(GwTrace))) == NULL) {
            fprintf(stderr, "Out of memory, can't add trace.\n");
            return (0);
        }

        t_end->flags = (TR_BLANK | TR_GRP_END);

        if (t_composite) {
            /* make the group end trace invisible */
            t_end->flags |= TR_COLLAPSED;
            t_end->name = (char *)malloc_2(1 + strlen("group_end"));
            strcpy(t_end->name, "group_end");
        } else {
            t_end->name = (char *)malloc_2(1 + strlen(name));
            strcpy(t_end->name, name);
        }

        GLOBALS->traces.bufferlast->t_next = t_end;
        t_end->t_prev = GLOBALS->traces.bufferlast;
        GLOBALS->traces.bufferlast = t_end;
        GLOBALS->traces.buffercount++;

        t_begin->t_match = t_end;
        t_end->t_match = t_begin;

        if (t_prev) {
            t_prev->flags |= TR_HIGHLIGHT;
            PasteBuffer();
        } else {
            PrependBuffer();
        }
    }
    return dirty;
}

static void create_group_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    unsigned dirty = 0;

    dirty = create_group(GLOBALS->entrybox_text, NULL);

    if (!dirty) {
        must_sel_bg();
    } else {
        GLOBALS->signalwindow_width_dirty = 1;
        redraw_signals_and_waves();
    }
}

void menu_create_group(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwTrace *t;
    unsigned dirty = 0;

    t = GLOBALS->traces.first;
    while (t) {
        if (t->flags & TR_HIGHLIGHT) {
            dirty = 1;
            break;
        }
        t = t->t_next;
    }

    if (dirty) {
        /* don't mess with sigs when dnd active */
        if (GLOBALS->dnd_state) {
            dnd_error();
            return;
        }
        entrybox("Create Group",
                 300,
                 "",
                 "Enter group name:",
                 128,
                 G_CALLBACK(create_group_cleanup));
    } else {
        must_sel_bg();
    }
}

static unsigned expand_trace(GwTrace *t_top)
{
    GwTrace *t;
    GwTrace *tmp;
    int tmpi;
    unsigned dirty = 0;
    int color;

    t = t_top;

    if (HasWave(t) && !IsGroupBegin(t) && !IsGroupEnd(t)) {
        FreeCutBuffer();
        GLOBALS->traces.buffer = GLOBALS->traces.first;
        GLOBALS->traces.bufferlast = GLOBALS->traces.last;
        GLOBALS->traces.buffercount = GLOBALS->traces.total;

        GLOBALS->traces.first = GLOBALS->traces.last = NULL;
        GLOBALS->traces.total = 0;

        color = t->t_color;

        if (t->vector) {
            GwBits *bits;
            int i;
            GwTrace *tfix;
            GwTime otime = t->shift;

            bits = t->n.vec->bits;
            if (!(t->flags & TR_REVERSE)) {
                for (i = 0; i < bits->nnbits; i++) {
                    if (bits->nodes[i]->expansion)
                        bits->nodes[i]->expansion->refcnt++;
                    GLOBALS->which_t_color = color;
                    AddNodeTraceReturn(bits->nodes[i], NULL, &tfix);
                    if (bits->attribs) {
                        tfix->shift = otime + bits->attribs[i].shift;
                    }
                }
            } else {
                for (i = (bits->nnbits - 1); i > -1; i--) {
                    if (bits->nodes[i]->expansion)
                        bits->nodes[i]->expansion->refcnt++;
                    GLOBALS->which_t_color = color;
                    AddNodeTraceReturn(bits->nodes[i], NULL, &tfix);
                    if (bits->attribs) {
                        tfix->shift = otime + bits->attribs[i].shift;
                    }
                }
            }
        } else {
            eptr e = ExpandNode(t->n.nd);
            int i;
            if (!e) {
                /* 		      if(t->n.nd->expansion) t->n.nd->expansion->refcnt++; */
                /* 		      AddNode(t->n.nd,NULL); */
            } else {
                int dhc_sav = GLOBALS->do_hier_compress;
                GLOBALS->do_hier_compress = 0;
                for (i = 0; i < e->width; i++) {
                    GLOBALS->which_t_color = color;
                    AddNode(e->narray[i], NULL);
                }
                GLOBALS->do_hier_compress = dhc_sav;
                free_2(e->narray);
                free_2(e);
            }
        }

        tmp = GLOBALS->traces.buffer;
        GLOBALS->traces.buffer = GLOBALS->traces.first;
        GLOBALS->traces.first = tmp;
        tmp = GLOBALS->traces.bufferlast;
        GLOBALS->traces.bufferlast = GLOBALS->traces.last;
        GLOBALS->traces.last = tmp;
        tmpi = GLOBALS->traces.buffercount;
        GLOBALS->traces.buffercount = GLOBALS->traces.total;
        GLOBALS->traces.total = tmpi;

        if (GLOBALS->traces.buffercount > 0) {
            /* buffer now contains the created signals */
            ClearTraces();

            if (t_top->t_prev) {
                t_top->t_prev->flags |= TR_HIGHLIGHT;
                RemoveTrace(t_top, 0);
                PasteBuffer();
                t_top->t_prev->flags &= ~TR_HIGHLIGHT;
            } else {
                RemoveTrace(t_top, 0);
                PrependBuffer();
            }

            dirty = create_group("unused_2", t_top);
        }
    }

    GLOBALS->which_t_color = 0;
    return dirty;
}

void menu_expand(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwTrace *t;
    GwTrace *t_next;
    int dirty = 0;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    DEBUG(printf("Expand Traces\n"));

    t = GLOBALS->traces.first;
    while (t) {
        if (IsSelected(t) && HasWave(t)) {
            t->flags |= TR_COLLAPSED;
            dirty = 1;
        }
        t = t->t_next;
    }

    if (dirty) {
        ClearTraces();
        t = GLOBALS->traces.first;
        while (t) {
            t_next = t->t_next;
            if (HasWave(t) && (t->flags & TR_COLLAPSED)) {
                if (!IsGroupBegin(t) && !IsGroupEnd(t)) {
                    expand_trace(t);
                } else {
                    OpenTrace(t);
                }
            }
            t = t_next;
        }
        t = GLOBALS->traces.first;
        if (t) {
            t->t_grp = NULL;
        } /* scan-build */
        while (t) {
            if (HasWave(t) && (t->flags & TR_COLLAPSED)) {
                t->flags &= ~TR_COLLAPSED;
                t->flags |= TR_HIGHLIGHT;
            }

            updateTraceGroup(t);
            t = t->t_next;
        }

        t = GLOBALS->traces.first;
        while (t) {
            if (IsSelected(t)) {
                break;
            }
            t = t->t_next;
        }

        GLOBALS->signalwindow_width_dirty = 1;
        redraw_signals_and_waves();
    } else {
        must_sel_nb();
    }
}

void menu_toggle_group(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwTrace *t;
    unsigned dirty_group = 0;
    unsigned dirty_signal = 0;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    t = GLOBALS->traces.first;
    while (t) {
        if ((t->flags & TR_HIGHLIGHT) && (IsGroupBegin(t) || IsGroupEnd(t))) {
            dirty_group = 1;
            break;
        }

        if ((t->flags & TR_HIGHLIGHT) && HasWave(t)) {
            dirty_signal = 1;
            break;
        }
        t = t->t_next;
    }

    if (dirty_group) {
        if (IsClosed(t)) {
            menu_open_group(widget, null_data);
            gtkwavetcl_setvar(WAVE_TCLCB_OPEN_TRACE_GROUP,
                              t->name,
                              WAVE_TCLCB_OPEN_TRACE_GROUP_FLAGS);
        } else {
            menu_close_group(widget, null_data);
            gtkwavetcl_setvar(WAVE_TCLCB_CLOSE_TRACE_GROUP,
                              t->name,
                              WAVE_TCLCB_CLOSE_TRACE_GROUP_FLAGS);
        }
        return;
    }
    if (dirty_signal) {
        ClearTraces();
        t->flags |= TR_HIGHLIGHT;
        menu_expand(null_data, 0, widget);
        gtkwavetcl_setvar(WAVE_TCLCB_OPEN_TRACE_GROUP, t->name, WAVE_TCLCB_OPEN_TRACE_GROUP_FLAGS);
        return;
    }

    must_sel_bg();
}

static void rename_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    GwTrace *t = GLOBALS->trace_to_alias_menu_c_1;

    if (GLOBALS->entrybox_text) {
        char *efix;
        /* code to turn '{' and '}' into '[' and ']' */
        if (!(t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH))) {
            efix = GLOBALS->entrybox_text;
            while (*efix) {
                if (*efix == '{') {
                    *efix = '[';
                }
                if (*efix == '}') {
                    *efix = ']';
                }
                efix++;
            }
        }

        if (t->vector) {
            if (t->name_full) {
                free_2(t->name_full);
                t->name_full = NULL;
            }
            if (t->n.vec->bvname) {
                free_2(t->n.vec->bvname);
            }

            t->n.vec->bvname = (char *)malloc_2(1 + strlen(GLOBALS->entrybox_text));
            strcpy(t->n.vec->bvname, GLOBALS->entrybox_text);
            t->name = t->n.vec->bvname;
            if (GLOBALS->hier_max_level)
                t->name = hier_extract(t->name, GLOBALS->hier_max_level);

            t->flags &= ~TR_HIGHLIGHT;
        }

        GLOBALS->signalwindow_width_dirty = 1;
        redraw_signals_and_waves();
    }
}

static void menu_rename(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    GwTrace *t;
    /* currently only called by various combine menu options, so no help menu text */

    GLOBALS->trace_to_alias_menu_c_1 = NULL;

    /* don't mess with sigs when dnd active */
    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    }

    t = GLOBALS->traces.first;
    while (t) {
        if (IsSelected(t) && (t->vector == TRUE)) {
            GLOBALS->trace_to_alias_menu_c_1 = t;
            break;
        }
        t = t->t_next;
    }

    if (GLOBALS->trace_to_alias_menu_c_1) {
        int was_packed = HIER_DEPACK_ALLOC;
        char *current = GetFullName(GLOBALS->trace_to_alias_menu_c_1, &was_packed);
        ClearTraces();
        GLOBALS->trace_to_alias_menu_c_1->flags |= TR_HIGHLIGHT;
        entrybox("Trace Name", 300, current, NULL, 128, G_CALLBACK(rename_cleanup));
        if (was_packed) {
            free_2(current);
        }
    } else {
        must_sel();
    }
}

GwBitVector *combine_traces(int direction, GwTrace *single_trace_only)
{
    GwTrace *t;
    GwTrace *tmp;
    GwTrace *tfirst = NULL;
    int tmpi, dirty = 0, attrib_reqd = 0;
    GwNode *bitblast_parent;
    int bitblast_delta = 0;

    DEBUG(printf("Combine Traces\n"));

    t = single_trace_only ? single_trace_only : GLOBALS->traces.first;
    while (t) {
        if ((t->flags & TR_HIGHLIGHT) && (!(t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH)))) {
            if (t->vector) {
                dirty += t->n.vec->nbits;
            } else {
                if (t->n.nd->extvals) {
                    int msb, lsb, width;
                    msb = t->n.nd->msi;
                    lsb = t->n.nd->lsi;
                    if (msb > lsb)
                        width = msb - lsb + 1;
                    else
                        width = lsb - msb + 1;
                    dirty += width;
                } else {
                    dirty++;
                }
            }
        }
        if (t == single_trace_only)
            break;
        t = t->t_next;
    }

    if (!dirty) {
        if (!single_trace_only)
            must_sel_nb();
        return NULL;
    }
    if (dirty > BITATTRIBUTES_MAX) {
        char buf[128];

        if (!single_trace_only) {
            sprintf(buf, "%d bits selected, please use <= %d.\n", dirty, BITATTRIBUTES_MAX);
            status_text(buf);
        }
        return NULL;
    } else {
        int i, nodepnt = 0;
        GwNode *n[BITATTRIBUTES_MAX];
        GwBitAttributes ba[BITATTRIBUTES_MAX];
        GwBits *b = NULL;
        GwBitVector *v = NULL;

        memset(n, 0, sizeof(n)); /* scan-build */

        if (!single_trace_only) {
            FreeCutBuffer();
            GLOBALS->traces.buffer = GLOBALS->traces.first;
            GLOBALS->traces.bufferlast = GLOBALS->traces.last;
            GLOBALS->traces.buffercount = GLOBALS->traces.total;

            GLOBALS->traces.first = GLOBALS->traces.last = NULL;
            GLOBALS->traces.total = 0;

            t = GLOBALS->traces.buffer;
        } else {
            t = single_trace_only;
        }

        tfirst = t;
        while (t) {
            if (t->flags & TR_HIGHLIGHT) {
                if (t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH)) {
                    /* nothing */
                } else {
                    if (t->vector) {
                        int ix;
                        GwBits *bits = t->n.vec->bits;
                        GwBitAttributes *oldba = bits ? bits->attribs : NULL;

                        bits = t->n.vec->bits;

                        if (!(t->flags & TR_REVERSE)) {
                            for (ix = 0; ix < bits->nnbits; ix++) {
                                if (bits->nodes[ix]->expansion)
                                    bits->nodes[ix]->expansion->refcnt++;
                                ba[nodepnt].shift = t->shift + (oldba ? oldba[ix].shift : 0);
                                ba[nodepnt].flags =
                                    t->flags ^ (oldba ? oldba[ix].flags & TR_INVERT : 0);
                                n[nodepnt++] = bits->nodes[ix];
                            }
                        } else {
                            for (ix = (bits->nnbits - 1); ix > -1; ix--) {
                                if (bits->nodes[ix]->expansion)
                                    bits->nodes[ix]->expansion->refcnt++;
                                ba[nodepnt].shift = t->shift + (oldba ? oldba[ix].shift : 0);
                                ba[nodepnt].flags =
                                    t->flags ^ (oldba ? oldba[ix].flags & TR_INVERT : 0);
                                n[nodepnt++] = bits->nodes[ix];
                            }
                        }
                    } else {
                        eptr e = ExpandNode(t->n.nd);
                        int ix;
                        if (!e) {
                            if (t->n.nd->expansion)
                                t->n.nd->expansion->refcnt++;
                            ba[nodepnt].shift = t->shift;
                            ba[nodepnt].flags = t->flags;
                            n[nodepnt++] = t->n.nd;
                        } else {
                            for (ix = 0; ix < e->width; ix++) {
                                ba[nodepnt].shift = t->shift;
                                ba[nodepnt].flags = t->flags;
                                n[nodepnt++] = e->narray[ix];
                                e->narray[ix]->expansion->refcnt++;
                            }
                            free_2(e->narray);
                            free_2(e);
                        }
                    }
                }
            }
            if (nodepnt == dirty)
                break;
            if (t == single_trace_only)
                break;
            t = t->t_next;
        }

        b = calloc_2(1, sizeof(GwBits) + (nodepnt) * sizeof(GwNode *));

        b->attribs = malloc_2(nodepnt * sizeof(GwBitAttributes));
        for (i = 0; i < nodepnt; i++) /* for up combine we need to reverse the attribs list! */
        {
            if (direction) {
                memcpy(b->attribs + i, ba + i, sizeof(GwBitAttributes));
            } else {
                memcpy(b->attribs + i, ba + (nodepnt - 1 - i), sizeof(GwBitAttributes));
            }

            if ((ba[i].shift) ||
                (ba[i].flags & TR_INVERT)) /* timeshift/invert are only relevant flags */
            {
                attrib_reqd = 1;
            }
        }

        if (!attrib_reqd) {
            free_2(b->attribs);
            b->attribs = NULL;
        }

        if (nodepnt && n[0] && n[0]->expansion) /* scan-build */
        {
            bitblast_parent = n[0]->expansion->parent;
        } else {
            bitblast_parent = NULL;
        }

        if (direction) {
            for (i = 0; i < nodepnt; i++) {
                b->nodes[i] = n[i];
                if (n[i] && n[i]->expansion) /* scan-build */
                {
                    if (bitblast_parent != n[i]->expansion->parent) {
                        bitblast_parent = NULL;
                    } else {
                        if (i == 1) {
                            if (n[0] && n[0]->expansion) /* scan-build */
                            {
                                bitblast_delta = n[1]->expansion->actual - n[0]->expansion->actual;
                                if (bitblast_delta < -1)
                                    bitblast_delta = 0;
                                else if (bitblast_delta > 1)
                                    bitblast_delta = 0;
                            } else {
                                bitblast_delta = 0;
                            }
                        } else if ((bitblast_delta) && (i > 1)) {
                            if ((n[i]->expansion->actual - n[i - 1]->expansion->actual) !=
                                bitblast_delta)
                                bitblast_delta = 0;
                        }
                    }
                } else {
                    bitblast_parent = NULL;
                }
            }
        } else {
            int rev;
            rev = nodepnt - 1;
            for (i = 0; i < nodepnt; i++) {
                b->nodes[i] = n[rev--];
                if (n[i] && n[i]->expansion) /* scan-build */
                {
                    if (bitblast_parent != n[i]->expansion->parent) {
                        bitblast_parent = NULL;
                    } else {
                        if (i == 1) {
                            if (n[0] && n[0]->expansion) /* scan-build */
                            {
                                bitblast_delta = n[1]->expansion->actual - n[0]->expansion->actual;
                                if (bitblast_delta < -1)
                                    bitblast_delta = 0;
                                else if (bitblast_delta > 1)
                                    bitblast_delta = 0;
                            } else {
                                bitblast_delta = 0;
                            }
                        } else if ((bitblast_delta) && (i > 1)) {
                            if ((n[i]->expansion->actual - n[i - 1]->expansion->actual) !=
                                bitblast_delta)
                                bitblast_delta = 0;
                        }
                    }
                } else {
                    bitblast_parent = NULL;
                }
            }
        }

        b->nnbits = nodepnt;

        if (!bitblast_parent) {
            char *aname;
            int match_iter = 1;

            if (direction) {
                aname = (nodepnt && n[0]) ? attempt_vecmatch(n[0]->nname, n[nodepnt - 1]->nname)
                                          : NULL; /* scan-build */
            } else {
                aname = (nodepnt && n[0]) ? attempt_vecmatch(n[nodepnt - 1]->nname, n[0]->nname)
                                          : NULL; /* scan-build */
            }

            if (aname) {
                int ix;

                for (ix = 0; ix < nodepnt - 1; ix++) {
                    char *mat = attempt_vecmatch(n[0]->nname, n[ix]->nname);
                    if (!mat) {
                        match_iter = 0;
                        break;
                    } else {
                        free_2(mat);
                    }
                }
            }

            if (!match_iter) {
                free_2(aname);
                aname = NULL;
            }

            if ((!aname) && !single_trace_only) /* ajb 020716: recent add to handle combine down on
                                                   2D vectors */
            {
                char *t_topname = NULL;
                char *t_botname = NULL;
                t = tfirst;
                while (t) {
                    if (t->flags & TR_HIGHLIGHT) {
                        if (t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH)) {
                            /* nothing */
                        } else {
                            if (t->vector) {
                                char *vname = t->n.vec ? t->n.vec->bvname : NULL;
                                if (vname) {
                                    if (!t_topname) {
                                        t_topname = vname;
                                    } else {
                                        t_botname = vname;
                                    }
                                }
                            } else {
                                if (!t_topname) {
                                    t_topname = t->n.nd->nname;
                                } else {
                                    t_botname = t->n.nd->nname;
                                }

                                if (t_topname && t_botname) {
                                    char *mat = attempt_vecmatch(t_topname, t_botname);
                                    if (!mat) {
                                        t_topname = t_botname = NULL;
                                        break;
                                    } else {
                                        free_2(mat);
                                    }
                                }
                            }
                        }
                    }

                    t = t->t_next;
                }

                if (t_topname && t_botname) {
                    aname = direction
                                ? attempt_vecmatch(t_topname, t_botname)
                                : attempt_vecmatch(t_botname, t_topname); /* return this match */
                }
            }

            if (!b->attribs) {
                if (aname) {
                    b->name = aname;
                } else {
                    strcpy(b->name = (char *)malloc_2(strlen("<Vector>") + 1), "<Vector>");
                }
            } else {
                if (aname) {
                    b->name = aname;
                } else {
                    strcpy(b->name = (char *)malloc_2(strlen("<ComplexVector>") + 1),
                           "<ComplexVector>");
                }
            }
        } else {
            int ix, offset;
            char *nam;
            char *namex;
            int was_packed = HIER_DEPACK_ALLOC;

            int row = 0, bit = 0;
            int row2 = 0, bit2 = 0;
            int is_2d = 0;
            int iy, offsety;
            char *namey;
            char sep2d = ':';

            namex = hier_decompress_flagged(n[0]->nname, &was_packed);

            offset = strlen(namex);
            for (ix = offset - 1; ix >= 0; ix--) {
                if (namex[ix] == '[')
                    break;
            }
            if (ix > -1)
                offset = ix;

            if (ix > 3) /* is_2d is for handling 2-d stored in 1-d vector */
            {
                if (namex[ix - 1] == ']') {
                    int j = ix - 2;
                    for (; j >= 0; j--) {
                        if (namex[j] == '[')
                            break;
                    }

                    if (j > -1) {
                        int items = sscanf(namex + j, "[%d][%d]", &row, &bit);
                        if (items == 2) {
                            /* printf(">> %d %d (items = %d)\n", row, bit, items); */

                            offset = j;
                            is_2d = 1;
                        }
                    }
                }
            }

            nam = (char *)g_alloca(offset + 50); /* to handle [a:b][c:d] case */
            memcpy(nam, namex, offset);
            if (was_packed) {
                free_2(namex);
            }

            if (is_2d) {
                is_2d = 0;
                namey = hier_decompress_flagged(n[nodepnt - 1]->nname, &was_packed);

                offsety = strlen(namey);
                for (iy = offsety - 1; iy >= 0; iy--) {
                    if (namey[iy] == '[')
                        break;
                }
                /* if(iy>-1) offsety=iy; not needed as in above as gets overwritten below by offsety
                 * = j */
                if (iy > 3) {
                    if (namey[iy - 1] == ']') {
                        int j = iy - 2;
                        for (; j >= 0; j--) {
                            if (namey[j] == '[')
                                break;
                        }

                        if (j > -1) {
                            int items = sscanf(namey + j, "[%d][%d]", &row2, &bit2);
                            if (items == 2) {
                                int rowabs, bitabs, width2d;
                                /* printf(">> %d %d (items = %d)\n", row2, bit2, items); */

                                offsety = j;
                                is_2d = (offset == offsety) && !memcmp(nam, namey, offsety);

                                rowabs = (row2 > row) ? (row2 - row + 1) : (row - row2 + 1);
                                bitabs = (bit2 > bit) ? (bit2 - bit + 1) : (bit - bit2 + 1);
                                width2d = rowabs * bitabs;
                                sep2d = (width2d == nodepnt) ? ':' : '|';
                            }
                        }
                    }
                }

                if (was_packed) {
                    free_2(namey);
                }
            }

            if (direction) {
                if (!is_2d) {
                    sprintf(nam + offset,
                            "[%d%s%d]",
                            n[0]->expansion->actual,
                            (bitblast_delta != 0) ? ":" : "|",
                            n[nodepnt - 1]->expansion->actual);
                } else {
                    if (row == row2) {
                        if (bit == bit2) {
                            sprintf(nam + offset, "[%d][%d]", row, bit);
                        } else {
                            sprintf(nam + offset, "[%d][%d%c%d]", row, bit, sep2d, bit2);
                        }
                    } else {
                        if (bit == bit2) {
                            sprintf(nam + offset, "[%d%c%d][%d]", row, sep2d, row2, bit);
                        } else {
                            sprintf(nam + offset,
                                    "[%d%c%d][%d%c%d]",
                                    row,
                                    sep2d,
                                    row2,
                                    bit,
                                    sep2d,
                                    bit2);
                        }
                    }
                }
            } else {
                if (!is_2d) {
                    sprintf(nam + offset,
                            "[%d%s%d]",
                            n[nodepnt - 1]->expansion->actual,
                            (bitblast_delta != 0) ? ":" : "|",
                            n[0]->expansion->actual);
                } else {
                    if (row == row2) {
                        if (bit == bit2) {
                            sprintf(nam + offset, "[%d][%d]", row, bit);
                        } else {
                            sprintf(nam + offset, "[%d][%d%c%d]", row, bit2, sep2d, bit);
                        }
                    } else {
                        if (bit == bit2) {
                            sprintf(nam + offset, "[%d%c%d][%d]", row2, sep2d, row, bit);
                        } else {
                            sprintf(nam + offset,
                                    "[%d%c%d][%d%c%d]",
                                    row2,
                                    sep2d,
                                    row,
                                    bit2,
                                    sep2d,
                                    bit);
                        }
                    }
                }
            }

            strcpy(b->name = (char *)malloc_2(offset + strlen(nam + offset) + 1), nam);
            DEBUG(printf("Name is: '%s'\n", nam));
        }

        if ((v = bits2vector(b))) {
            v->bits = b; /* only needed for savefile function */
        } else {
            free_2(b->name);
            if (b->attribs)
                free_2(b->attribs);
            free_2(b);
            return NULL;
        }

        if (!single_trace_only) {
            tmp = GLOBALS->traces.buffer;
            GLOBALS->traces.buffer = GLOBALS->traces.first;
            GLOBALS->traces.first = tmp;
            tmp = GLOBALS->traces.bufferlast;
            GLOBALS->traces.bufferlast = GLOBALS->traces.last;
            GLOBALS->traces.last = tmp;
            tmpi = GLOBALS->traces.buffercount;
            GLOBALS->traces.buffercount = GLOBALS->traces.total;
            GLOBALS->traces.total = tmpi;
            PasteBuffer();
        }

        return v;
    }
}

void menu_combine_down(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwBitVector *v;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */
    v = combine_traces(1, NULL); /* down */

    if (v) {
        GwTrace *t;

        AddVector(v, NULL);
        free_2(v->bits->name);
        v->bits->name = NULL;

        t = GLOBALS->traces.last;

        RemoveTrace(t, 0);

        /* t is now the composite signal trace */

        create_group("unused_0", t);

        GLOBALS->signalwindow_width_dirty = 1;
        redraw_signals_and_waves();

        menu_rename(widget, null_data);
    } else {
    }
}

void menu_combine_up(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwBitVector *v;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */
    v = combine_traces(0, NULL); /* up */

    if (v) {
        GwTrace *t;

        AddVector(v, NULL);
        free_2(v->bits->name);
        v->bits->name = NULL;

        t = GLOBALS->traces.last;

        RemoveTrace(t, 0);

        /* t is now the composite signal trace */

        create_group("unused_1", t);

        GLOBALS->signalwindow_width_dirty = 1;
        redraw_signals_and_waves();

        menu_rename(widget, null_data);
    } else {
    }
}

/**/

void menu_tracesearchbox_callback(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
}

void menu_tracesearchbox(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwTrace *t;

    for (t = GLOBALS->traces.first; t; t = t->t_next) {
        if ((t->flags & TR_HIGHLIGHT) && HasWave(t)) {
            /* at least one good trace, so do it */
            /* data contains WV_MENU_SPS or WV_MENU_SPS2 or ... but the base is WV_MENU_SPS*/
            char buf[128];
            intptr_t which = ((long)callback_action) - WV_MENU_SPS;

            if ((which < 0) || (which >= WAVE_NUM_STRACE_WINDOWS)) { /* should never happen unless
                                                                        menus are defined wrong */
                sprintf(buf,
                        "Pattern search ID %d out of range of 1-%d available, ignoring.",
                        (int)(which + 1),
                        WAVE_NUM_STRACE_WINDOWS);
                status_text(buf);
            } else {
                sprintf(buf, "Waveform Display Search (%d)", (int)(which + 1));
                tracesearchbox(buf, G_CALLBACK(menu_tracesearchbox_callback), (gpointer)which);
            }
            return;
        }
    }
    must_sel();
}

/**/
void menu_new_viewer_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    pid_t pid;

    if (GLOBALS->filesel_ok) {
        /* fix problem where ungrab doesn't occur if button pressed + simultaneous accelerator key
         * occurs */
        if (GLOBALS->in_button_press_wavewindow_c_1) {
            XXX_gdk_pointer_ungrab(GDK_CURRENT_TIME);
        }

#if !defined __MINGW32__
        /*
         * for some reason, X won't let us double-fork in order to cleanup zombies.. *shrug*
         */
        pid = fork();
        if (((int)pid) < 0) {
            return; /* not much we can do about this.. */
        }

        if (pid) /* parent==original server_pid */
        {
            return;
        }

#ifdef MAC_INTEGRATION
        /* from : @pfx = split(' ', "open -n -W -a gtkwave --args --chdir dummy"); */
        if (GLOBALS->optimize_vcd) {
            execlp("open",
                   "open",
                   "-n",
                   "-W",
                   "-a",
                   "gtkwave",
                   "--args",
                   "--optimize",
                   "--dump",
                   *GLOBALS->fileselbox_text,
                   NULL);
        } else {
            execlp("open",
                   "open",
                   "-n",
                   "-W",
                   "-a",
                   "gtkwave",
                   "--args",
                   "--dump",
                   *GLOBALS->fileselbox_text,
                   NULL);
        }
#else
        if (GLOBALS->optimize_vcd) {
            execlp(GLOBALS->whoami, GLOBALS->whoami, "-o", *GLOBALS->fileselbox_text, NULL);
        } else {
            execlp(GLOBALS->whoami, GLOBALS->whoami, *GLOBALS->fileselbox_text, NULL);
        }
#endif
        exit(0); /* control never gets here if successful */

#else

        BOOL bSuccess = FALSE;
        PROCESS_INFORMATION piProcInfo;
        TCHAR *szCmdline;
        STARTUPINFO si;

        memset(&piProcInfo, 0, sizeof(PROCESS_INFORMATION));
        memset(&si, 0, sizeof(STARTUPINFO));

        szCmdline = malloc_2(strlen(GLOBALS->whoami) + 1 + strlen(*GLOBALS->fileselbox_text) + 1);
        sprintf(szCmdline, "%s %s", GLOBALS->whoami, *GLOBALS->fileselbox_text);

        bSuccess = CreateProcess(NULL,
                                 szCmdline, /* command line */
                                 NULL, /* process security attributes */
                                 NULL, /* primary thread security attributes */
                                 TRUE, /* handles are inherited */
                                 0, /* creation flags */
                                 NULL, /* use parent's environment */
                                 NULL, /* use parent's current directory */
                                 &si, /* STARTUPINFO pointer */
                                 &piProcInfo); /* receives PROCESS_INFORMATION */

        free_2(szCmdline);

        if (!bSuccess) {
            /* failed */
        }
#endif
    }
}

void menu_new_viewer(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    static int mnv = 0;

    if (!mnv && !GLOBALS->busy_busy_c_1) {
        mnv = 1;
        if (in_main_iteration()) {
            mnv = 0;
            return;
        }

        fileselbox("Select a trace to view...",
                   &GLOBALS->filesel_newviewer_menu_c_1,
                   G_CALLBACK(menu_new_viewer_cleanup),
                   G_CALLBACK(NULL),
                   NULL,
                   0);
        mnv = 0;
    }
}

/**/

int menu_new_viewer_tab_cleanup_2(char *fname, int optimize_vcd)
{
    int rc = 0;
    char *argv[2];
    struct Global *g_old = GLOBALS;
    struct Global *g_now;

    argv[0] = g_strdup(gtkwave_argv0_cached ? gtkwave_argv0_cached : "gtkwave");
    argv[1] = g_strdup(fname);

    /* fix problem where ungrab doesn't occur if button pressed + simultaneous accelerator key
     * occurs */
    if (GLOBALS->in_button_press_wavewindow_c_1) {
        XXX_gdk_pointer_ungrab(GDK_CURRENT_TIME);
    }

    GLOBALS->vcd_jmp_buf = calloc(1, sizeof(jmp_buf));

    splash_button_press_event(NULL,
                              NULL); /* kill any possible splash screens (e.g., if automated) */
    set_window_busy(NULL);
    gtkwave_main_iteration();

    if (!setjmp(*(GLOBALS->vcd_jmp_buf))) {
        main_2(optimize_vcd, 2, argv);

        g_free(argv[0]);
        g_free(argv[1]);

        g_now = GLOBALS;
        set_GLOBALS(g_old);

        free(GLOBALS->vcd_jmp_buf);
        GLOBALS->vcd_jmp_buf = NULL;
        set_window_idle(NULL);
        set_GLOBALS(g_now);
        g_now->vcd_jmp_buf = NULL;

        /* copy old file req strings into new context */
        strcpy2_into_new_context(GLOBALS,
                                 &GLOBALS->fcurr_ttranslate_c_1,
                                 &g_old->fcurr_ttranslate_c_1);
        strcpy2_into_new_context(GLOBALS,
                                 &GLOBALS->fcurr_ptranslate_c_1,
                                 &g_old->fcurr_ptranslate_c_1);
        strcpy2_into_new_context(GLOBALS,
                                 &GLOBALS->fcurr_translate_c_2,
                                 &g_old->fcurr_translate_c_2);

#if 0
		/* disabled for now...these probably would be disruptive */
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_lxt_writesave, &g_old->filesel_lxt_writesave);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_vcd_writesave, &g_old->filesel_vcd_writesave);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_tim_writesave, &g_old->filesel_tim_writesave);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_writesave, &g_old->filesel_writesave);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->stems_name, &g_old->stems_name);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_logfile_menu_c_1, &g_old->filesel_logfile_menu_c_1);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_scriptfile_menu, &g_old->filesel_scriptfile_menu);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_print_pdf_renderopt_c_1, &g_old->filesel_print_pdf_renderopt_c_1);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_print_ps_renderopt_c_1, &g_old->filesel_print_ps_renderopt_c_1);
		strcpy2_into_new_context(GLOBALS, &GLOBALS->filesel_print_mif_renderopt_c_1, &g_old->filesel_print_mif_renderopt_c_1);
#endif

        /* not sure what's really needed here */
        /* for now, add back in repscript_name */
        GLOBALS->repscript_period = g_old->repscript_period;
        strcpy2_into_new_context(GLOBALS, &GLOBALS->repscript_name, &g_old->repscript_name);

        GLOBALS->strace_repeat_count = g_old->strace_repeat_count;

        if (g_old->loaded_file_type == MISSING_FILE) /* remove original "blank" page */
        {
            if (g_old->missing_file_toolbar) {
                gtk_widget_set_sensitive(g_old->missing_file_toolbar, TRUE);
            }
            menu_set_sensitive();
            gtk_notebook_set_current_page(GTK_NOTEBOOK(g_old->notebook), g_old->this_context_page);
            set_GLOBALS(g_old);
            menu_quit_close_callback(NULL, NULL);
        }

        wave_gconf_client_set_string("/current/pwd", getenv("PWD"));

        wave_gconf_client_set_string("/current/dumpfile",
                                     GLOBALS->optimize_vcd ? GLOBALS->unoptimized_vcd_file_name
                                                           : GLOBALS->loaded_file_name);
        wave_gconf_client_set_string("/current/optimized_vcd", GLOBALS->optimize_vcd ? "1" : "0");

        wave_gconf_client_set_string("/current/savefile", GLOBALS->filesel_writesave);

        rc = 1;
    } else {
        free_outstanding(); /* free anything allocated in loader ctx */
        free(GLOBALS);
        GLOBALS = NULL; /* valgrind fix */

        set_GLOBALS(g_old);
        free(GLOBALS->vcd_jmp_buf);
        GLOBALS->vcd_jmp_buf = NULL;
        set_window_idle(NULL);

        /* load failed */
        wave_gtk_window_set_title(GTK_WINDOW(GLOBALS->mainwindow),
                                  GLOBALS->winname,
                                  GLOBALS->dumpfile_is_modified ? WAVE_SET_TITLE_MODIFIED
                                                                : WAVE_SET_TITLE_NONE,
                                  0);
        printf("GTKWAVE | File load failure, new tab not created.\n");
        rc = 0;
    }

    return (rc);
}

void menu_new_viewer_tab_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    if (GLOBALS->filesel_ok) {
        menu_new_viewer_tab_cleanup_2(*GLOBALS->fileselbox_text, GLOBALS->optimize_vcd);
    }
}

void menu_new_viewer_tab(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (in_main_iteration())
        return;

    if ((!GLOBALS->socket_xid) && (!GLOBALS->partial_vcd)) {
        fileselbox("Select a trace to view...",
                   &GLOBALS->filesel_newviewer_menu_c_1,
                   G_CALLBACK(menu_new_viewer_tab_cleanup),
                   G_CALLBACK(NULL),
                   NULL,
                   0);
    }
}

/**/

void menu_reload_waveform(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (in_main_iteration())
        return;

    if (GLOBALS->gt_splash_c_1 || GLOBALS->splash_is_loading) {
        return; /* don't attempt reload if splash screen is still active...that's pointless anyway
                 */
    }

    /* XXX if there's no file (for some reason), this function shouldn't occur
       we should probably gray it out. */
    if (GLOBALS->loaded_file_type == DUMPLESS_FILE) {
        printf("GTKWAVE | DUMPLESS_FILE type cannot be reloaded\n");
        return;
    }

    reload_into_new_context();
}

void menu_reload_waveform_marshal(GtkWidget *widget, gpointer data)
{
    menu_reload_waveform(data, 0, widget);
}

/**/

void menu_print(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    renderbox("Print Formatting Options");
}

/**/
void menu_markerbox_callback(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
}

void menu_markerbox(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwNamedMarkers *markers = gw_project_get_named_markers(GLOBALS->project);
    GtkWidget *dialog = gw_named_marker_dialog_new(markers);

    // GtkWidget *dialog = g_object_new(GTK_TYPE_DIALOG, "use-header-bar", 1, NULL);
    // gtk_dialog_add_button(GTK_DIALOG(dialog), "OK", GTK_RESPONSE_OK);
    // gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    // GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    // gtk_container_add(GTK_CONTAINER(content), gtk_label_new("label"));
    // gtk_widget_show_all(content);

    gtk_dialog_run(GTK_DIALOG(dialog));

    // markerbox("Markers", G_CALLBACK(menu_markerbox_callback));
}

void copy_pri_b_marker(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    DEBUG(printf("copy_pri_b_marker()\n"));

    GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
    if (!gw_marker_is_enabled(primary_marker)) {
        return;
    }

    GwMarker *baseline_marker = gw_project_get_baseline_marker(GLOBALS->project);
    gw_marker_set_position(baseline_marker, gw_marker_get_position(primary_marker));
    gw_marker_set_enabled(baseline_marker, TRUE);

    update_time_box();
    GLOBALS->signalwindow_width_dirty = 1;
    redraw_signals_and_waves();
}

/**/
void delete_unnamed_marker(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    DEBUG(printf("delete_unnamed marker()\n"));

    GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
    if (!gw_marker_is_enabled(primary_marker)) {
        return;
    }

    GwTrace *t;

    for (t = GLOBALS->traces.first; t; t = t->t_next) {
        if (t->asciivalue) {
            free_2(t->asciivalue);
            t->asciivalue = NULL;
        }
    }

    for (t = GLOBALS->traces.buffer; t; t = t->t_next) {
        if (t->asciivalue) {
            free_2(t->asciivalue);
            t->asciivalue = NULL;
        }
    }

    gw_marker_set_enabled(primary_marker, FALSE);

    update_time_box();
    GLOBALS->signalwindow_width_dirty = 1;
    redraw_signals_and_waves();
}

static void disable_marker(gpointer data, gpointer user_data)
{
    GwMarker *marker = data;
    gboolean *dirty = user_data;

    if (!gw_marker_is_enabled(marker)) {
        return;
    }

    gw_marker_set_enabled(marker, FALSE);
    gw_marker_set_alias(marker, NULL);
    *dirty = TRUE;
}

/**/
void collect_all_named_markers(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwNamedMarkers *markers = gw_project_get_named_markers(GLOBALS->project);

    gboolean dirty = FALSE;
    gw_named_markers_foreach(markers, disable_marker, &dirty);

    if (dirty) {
        redraw_signals_and_waves();
    }
}
/**/
void collect_named_marker(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    DEBUG(printf("collect_named_marker()\n"));

    GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
    if (!gw_marker_is_enabled(primary_marker)) {
        return;
    }

    GwNamedMarkers *markers = gw_project_get_named_markers(GLOBALS->project);
    GwMarker *marker = gw_named_markers_find(markers, gw_marker_get_position(primary_marker));

    if (marker != NULL) {
        gw_marker_set_enabled(marker, FALSE);
        gw_marker_set_alias(marker, NULL);
        redraw_signals_and_waves();
    }
}
/**/
void drop_named_marker(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
    if (!gw_marker_is_enabled(primary_marker)) {
        return;
    }

    GwNamedMarkers *markers = gw_project_get_named_markers(GLOBALS->project);
    GwMarker *marker = gw_named_markers_find_first_disabled(markers);
    if (marker != NULL) {
        gw_marker_set_position(marker, gw_marker_get_position(primary_marker));
        gw_marker_set_enabled(marker, TRUE);
        redraw_signals_and_waves();
    }
}
/**/
void menu_showchangeall_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    GwTrace *t;
    TraceFlagsType flags;
    unsigned int t_color;

    t = GLOBALS->showchangeall_menu_c_1;
    if (t) {
        flags = t->flags & (TR_NUMMASK | TR_HIGHLIGHT | TR_ATTRIBS);
        t_color = t->t_color;
        while (t) {
            if ((t->flags & TR_HIGHLIGHT) && (!(t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH))) &&
                (t->name)) {
                t->flags = (t->flags & ~(TR_NUMMASK | TR_HIGHLIGHT | TR_ATTRIBS)) | flags;
                t->minmax_valid = 0; /* force analog traces to regenerate if necessary */
                t->t_color = t_color;
            }
            t = t->t_next;
        }
    }

    GLOBALS->signalwindow_width_dirty = 1;
    redraw_signals_and_waves();
    DEBUG(printf("menu_showchangeall_cleanup()\n"));
}

void menu_showchangeall(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwTrace *t;

    DEBUG(printf("menu_showchangeall()\n"));

    GLOBALS->showchangeall_menu_c_1 = NULL;
    t = GLOBALS->traces.first;
    while (t) {
        if ((t->flags & TR_HIGHLIGHT) && (!(t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH))) &&
            (t->name)) {
            showchange("Show-Change All",
                       GLOBALS->showchangeall_menu_c_1 = t,
                       G_CALLBACK(menu_showchangeall_cleanup));
            return;
        }
        t = t->t_next;
    }

    must_sel();
}

/**/
void menu_showchange_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    GLOBALS->signalwindow_width_dirty = 1;
    redraw_signals_and_waves();
    DEBUG(printf("menu_showchange_cleanup()\n"));
}

void menu_showchange(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwTrace *t;

    DEBUG(printf("menu_showchange()\n"));

    t = GLOBALS->traces.first;
    while (t) {
        if ((t->flags & TR_HIGHLIGHT) && (!(t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH))) &&
            (t->name)) {
            showchange("Show-Change", t, G_CALLBACK(menu_showchange_cleanup));
            return;
        }
        t = t->t_next;
    }

    must_sel();
}
/**/
void menu_remove_aliases(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwTrace *t;
    int dirty = 0, none_selected = 1;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    t = GLOBALS->traces.first;
    while (t) {
        if (HasAlias(t) && IsSelected(t)) {
            char *name_full;
            int was_packed = HIER_DEPACK_ALLOC;

            free_2(t->name_full);
            t->name_full = NULL;

            if (t->vector) {
                name_full = t->n.vec->bvname;
            } else {
                name_full = hier_decompress_flagged(t->n.nd->nname, &was_packed);
            }

            t->name = name_full;
            if (GLOBALS->hier_max_level) {
                if (!was_packed) {
                    t->name = hier_extract(t->name, GLOBALS->hier_max_level);
                } else {
                    t->name = strdup_2(hier_extract(name_full, GLOBALS->hier_max_level));
                    free_2(name_full);
                }
            }

            if (was_packed)
                t->is_depacked = 1;

            dirty = 1;
        }
        if (IsSelected(t))
            none_selected = 0;
        t = t->t_next;
    }

    if (dirty) {
        GLOBALS->signalwindow_width_dirty = 1;
        redraw_signals_and_waves();
        DEBUG(printf("menu_remove_aliases()\n"));
    }
    if (none_selected) {
        must_sel();
    }
}

static void alias_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    GwTrace *t = GLOBALS->trace_to_alias_menu_c_1;

    if (GLOBALS->entrybox_text) {
        char *efix;
        /* code to turn '{' and '}' into '[' and ']' */
        if (!(t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH))) {
            efix = GLOBALS->entrybox_text;
            while (*efix) {
                if (*efix == '{') {
                    *efix = '[';
                }
                if (*efix == '}') {
                    *efix = ']';
                }
                efix++;
            }
        }

        if (CanAlias(t)) {
            if (HasAlias(t))
                free_2(t->name_full);
            t->name_full = (char *)malloc_2(1 + strlen(GLOBALS->entrybox_text));
            strcpy(t->name_full, GLOBALS->entrybox_text);
            t->name = t->name_full;
            if (GLOBALS->hier_max_level)
                t->name = hier_extract(t->name, GLOBALS->hier_max_level);

            t->flags &= ~TR_HIGHLIGHT;
        }

        GLOBALS->signalwindow_width_dirty = 1;
        redraw_signals_and_waves();
    }
}

void menu_alias(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwTrace *t;

    GLOBALS->trace_to_alias_menu_c_1 = NULL;

    /* don't mess with sigs when dnd active */
    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    }

    t = GLOBALS->traces.first;
    while (t) {
        if (IsSelected(t) && CanAlias(t)) {
            GLOBALS->trace_to_alias_menu_c_1 = t;
            break;
        }
        t = t->t_next;
    }

    if (GLOBALS->trace_to_alias_menu_c_1) {
        int was_packed = HIER_DEPACK_ALLOC;
        char *current = GetFullName(GLOBALS->trace_to_alias_menu_c_1, &was_packed);
        ClearTraces();
        GLOBALS->trace_to_alias_menu_c_1->flags |= TR_HIGHLIGHT;
        entrybox("Alias Highlighted Trace", 300, current, NULL, 128, G_CALLBACK(alias_cleanup));
        if (was_packed) {
            free_2(current);
        }
    } else {
        must_sel();
    }
}

/**/
void menu_signalsearch_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    redraw_signals_and_waves();
    DEBUG(printf("menu_signalsearch_cleanup()\n"));
}

void menu_signalsearch(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    searchbox("Signal Search", G_CALLBACK(menu_signalsearch_cleanup));
}
/**/
static void regexp_highlight_generic(int mode)
{
    if (GLOBALS->entrybox_text) {
        GwTrace *t;
        Ulong modebits;
        char dirty = 0;

        modebits = (mode) ? TR_HIGHLIGHT : 0;

        strcpy(GLOBALS->regexp_string_menu_c_1, GLOBALS->entrybox_text);
        wave_regex_compile(GLOBALS->regexp_string_menu_c_1, WAVE_REGEX_SEARCH);
        free_2(GLOBALS->entrybox_text);
        t = GLOBALS->traces.first;
        while (t) {
            const char *pnt = (t->name) ? t->name : ""; /* handle (really) blank lines */

            if (*pnt == '+') /* skip alias prefix if present */
            {
                pnt++;
                if (*pnt == ' ') {
                    pnt++;
                }
            }

            if (wave_regex_match(pnt, WAVE_REGEX_SEARCH)) {
                t->flags = ((t->flags & (~TR_HIGHLIGHT)) | modebits);
                dirty = 1;
            }

            t = t->t_next;
        }

        if (dirty) {
            redraw_signals_and_waves();
        }
    }
}

static void regexp_unhighlight_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    regexp_highlight_generic(0);
}

void menu_regexp_unhighlight(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    entrybox("Regexp UnHighlight",
             300,
             GLOBALS->regexp_string_menu_c_1,
             NULL,
             128,
             G_CALLBACK(regexp_unhighlight_cleanup));
}
/**/
static void regexp_highlight_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    regexp_highlight_generic(1);
}

void menu_regexp_highlight(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    entrybox("Regexp Highlight",
             300,
             GLOBALS->regexp_string_menu_c_1,
             NULL,
             128,
             G_CALLBACK(regexp_highlight_cleanup));
}

/**/

void menu_write_screengrab_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    GdkWindow *gw;
    GdkPixbuf *pb = NULL;
    gboolean succ = FALSE;

    if (!GLOBALS->filesel_ok) {
        return;
    }

    gw = gtk_widget_get_window(GTK_WIDGET(GLOBALS->mainwindow));
    if (gw) {
        GtkAllocation allocation;
        int scale = gtk_widget_get_scale_factor(GTK_WIDGET(GLOBALS->mainwindow));
        gtk_widget_get_allocation(GLOBALS->mainwindow, &allocation);

        pb = gdk_pixbuf_get_from_window(gw, 0, 0, allocation.width, allocation.height);

        if (pb) {
            cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                                                  allocation.width * scale,
                                                                  allocation.height * scale);
            cairo_t *cr = cairo_create(surface);

            gdk_cairo_set_source_pixbuf(cr, pb, 0, 0);
            cairo_paint(cr);

            cairo_status_t rc = cairo_surface_write_to_png(surface, *GLOBALS->fileselbox_text);
            succ = (rc == CAIRO_STATUS_SUCCESS);

            g_object_unref(pb);
            cairo_surface_destroy(surface);
            cairo_destroy(cr);
        }
    }

    if (!succ) {
        fprintf(stderr,
                "Error opening imagegrab file '%s' for writing.\n",
                *GLOBALS->fileselbox_text);
        if (!pb) {
            fprintf(stderr, "Why: could not execute gdk_pixbuf_get_from_window().\n");
        } else {
            perror("Why");
        }
        errno = 0;
    } else {
        wave_gconf_client_set_string("/current/imagegrab", GLOBALS->filesel_imagegrab);
    }
}

void menu_write_screengrab_as(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    errno = 0;
    fileselbox("Grab To File",
               &GLOBALS->filesel_imagegrab,
               G_CALLBACK(menu_write_screengrab_cleanup),
               G_CALLBACK(NULL),
               "*.png",
               1);
}

/**/

void menu_write_save_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    FILE *wave;
    int len;

    if (!GLOBALS->filesel_ok) {
        return;
    }

    len = strlen(*GLOBALS->fileselbox_text);
    if ((!len) || ((*GLOBALS->fileselbox_text)[len - 1] == '/')
#if !defined __MINGW32__ && !defined _MSC_VER
        || ((*GLOBALS->fileselbox_text)[len - 1] == '\\')
#endif
    ) {
        GLOBALS->save_success_menu_c_1 = 2;
        return;
    }

    if (!(wave = fopen(*GLOBALS->fileselbox_text, "wb"))) {
        fprintf(stderr, "Error opening save file '%s' for writing.\n", *GLOBALS->fileselbox_text);
        perror("Why");
        errno = 0;
    } else {
        write_save_helper(*GLOBALS->fileselbox_text, wave);
        wave_gconf_client_set_string("/current/savefile", GLOBALS->filesel_writesave);
        GLOBALS->save_success_menu_c_1 = 1;
        fclose(wave);
    }
}

void menu_write_save_file_as(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    fileselbox("Write Save File",
               &GLOBALS->filesel_writesave,
               G_CALLBACK(menu_write_save_cleanup),
               G_CALLBACK(NULL),
               GLOBALS->is_gtkw_save_file ? "*.gtkw" : "*.sav",
               1);
}

void menu_write_save_file(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;
    int len = 0;

    if (GLOBALS->filesel_writesave) {
        len = strlen(GLOBALS->filesel_writesave);
    }

    if ((!len) || (GLOBALS->filesel_writesave[len - 1] == '/')
#if !defined __MINGW32__ && !defined _MSC_VER
        || (GLOBALS->filesel_writesave[len - 1] == '\\')
#endif
    ) {
        fileselbox("Write Save File",
                   &GLOBALS->filesel_writesave,
                   G_CALLBACK(menu_write_save_cleanup),
                   G_CALLBACK(NULL),
                   GLOBALS->is_gtkw_save_file ? "*.gtkw" : "*.sav",
                   1);
    } else {
        GLOBALS->filesel_ok = 1;
        GLOBALS->save_success_menu_c_1 = 0;
        GLOBALS->fileselbox_text = &GLOBALS->filesel_writesave;
        menu_write_save_cleanup(NULL, NULL);
        if (GLOBALS->save_success_menu_c_1 != 2) /* cancelled */
        {
            if (GLOBALS->save_success_menu_c_1) {
                status_text("Wrote save file OK.\n");
            } else {
                status_text("Problem writing save file.\n");
            }
        }
    }
}
/**/

void menu_read_save_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    if (GLOBALS->filesel_ok) {
        char *wname;

        DEBUG(printf("Read Save Fini: %s\n", *GLOBALS->fileselbox_text));

        wname = *GLOBALS->fileselbox_text;
        wave_gconf_client_set_string("/current/savefile", wname);
        read_save_helper(wname, NULL, NULL, NULL, NULL, NULL);
    }
}

void menu_read_save_file(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    fileselbox("Read Save File",
               &GLOBALS->filesel_writesave,
               G_CALLBACK(menu_read_save_cleanup),
               G_CALLBACK(NULL),
               GLOBALS->is_gtkw_save_file ? "*.gtkw" : "*.sav",
               0);
}

/**/
void menu_read_stems_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    char *fname;

    if (GLOBALS->filesel_ok) {
        DEBUG(printf("Read Stems Fini: %s\n", *GLOBALS->fileselbox_text));

        fname = *GLOBALS->fileselbox_text;
        if ((fname) && strlen(fname)) {
            activate_stems_reader(fname);
        }
    }
}
/**/
void menu_read_stems_file(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (!stems_are_active()) {
        if (GLOBALS->stems_type != WAVE_ANNO_NONE) {
            fileselbox("Read Verilog Stemsfile",
                       &GLOBALS->stems_name,
                       G_CALLBACK(menu_read_stems_cleanup),
                       G_CALLBACK(NULL),
                       NULL,
                       0);
        } else {
            status_text("Unsupported dumpfile type for rtlbrowse.\n");
        }
    }
}

/**/
void menu_read_log_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    char *fname;

    if (GLOBALS->filesel_ok) {
        DEBUG(printf("Read Log Fini: %s\n", *GLOBALS->fileselbox_text));

        fname = *GLOBALS->fileselbox_text;
        if ((fname) && strlen(fname)) {
            logbox("Logfile viewer", 480, fname);
        }
    }
}
/**/
void menu_read_log_file(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    fileselbox("Read Logfile",
               &GLOBALS->filesel_logfile_menu_c_1,
               G_CALLBACK(menu_read_log_cleanup),
               G_CALLBACK(NULL),
               NULL,
               0);
}

/**/
void menu_read_script_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    char *fname;

    if (GLOBALS->filesel_ok) {
        DEBUG(printf("Read Script Fini: %s\n", *GLOBALS->fileselbox_text));

        fname = *GLOBALS->fileselbox_text;
        if ((fname) && strlen(fname)) {
            execute_script(fname, 0);
        }
    }
}
/**/
void menu_read_script_file(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    fileselbox("Read Script File",
               &GLOBALS->filesel_scriptfile_menu,
               G_CALLBACK(menu_read_script_cleanup),
               G_CALLBACK(NULL),
               "*.tcl",
               0);
}

/**/
void menu_insert_blank_traces(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    DEBUG(printf("Insert Blank Trace\n"));

    InsertBlankTrace(NULL, 0);
    redraw_signals_and_waves();
}

void menu_insert_analog_height_extension(gpointer null_data,
                                         guint callback_action,
                                         GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    DEBUG(printf("Insert Analog Blank Trace\n"));

    InsertBlankTrace(NULL, TR_ANALOG_BLANK_STRETCH);
    redraw_signals_and_waves();
}
/**/
static void comment_trace_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    InsertBlankTrace(GLOBALS->entrybox_text, 0);
    if (GLOBALS->entrybox_text) {
        free_2(GLOBALS->entrybox_text);
        GLOBALS->entrybox_text = NULL;
    }
    GLOBALS->signalwindow_width_dirty = 1;
    redraw_signals_and_waves();
}

void menu_insert_comment_traces(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    DEBUG(printf("Insert Comment Trace\n"));

    entrybox("Insert Comment Trace", 300, "", NULL, 128, G_CALLBACK(comment_trace_cleanup));
}
/**/
static void strace_repcnt_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    if (GLOBALS->entrybox_text) {
        GLOBALS->strace_repeat_count = atoi_64(GLOBALS->entrybox_text);

        free_2(GLOBALS->entrybox_text);
        GLOBALS->entrybox_text = NULL;
    }
}

void menu_strace_repcnt(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    char gt[32];

    sprintf(gt, "%d", GLOBALS->strace_repeat_count);
    entrybox("Repeat Count", 300, gt, NULL, 20, G_CALLBACK(strace_repcnt_cleanup));
}
/**/
void movetotime_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    if (GLOBALS->entrybox_text) {
        GwTime gt = GLOBALS->tims.first;
        char update_string[128];
        char timval[40];
        GtkAdjustment *hadj;
        GwTime pageinc;

        if ((GLOBALS->entrybox_text[0] >= 'A' && GLOBALS->entrybox_text[0] <= 'Z') ||
            (GLOBALS->entrybox_text[0] >= 'a' && GLOBALS->entrybox_text[0] <= 'z')) {
            char *su = GLOBALS->entrybox_text;
            int uch;
            while (*su) {
                uch = toupper((int)(unsigned char)*su);
                *su = uch;
                su++;
            }

            uch = bijective_marker_id_string_hash(GLOBALS->entrybox_text);

            GwNamedMarkers *markers = gw_project_get_named_markers(GLOBALS->project);
            GwMarker *marker = gw_named_markers_get(markers, uch);
            if (marker != NULL && gw_marker_is_enabled(marker)) {
                gt = gw_marker_get_position(marker);
            }

        } else {
            gt = unformat_time(GLOBALS->entrybox_text, GLOBALS->time_dimension);
            gt -= GLOBALS->global_time_offset;
        }
        free_2(GLOBALS->entrybox_text);
        GLOBALS->entrybox_text = NULL;

        if (gt < GLOBALS->tims.first)
            gt = GLOBALS->tims.first;
        else if (gt > GLOBALS->tims.last)
            gt = GLOBALS->tims.last;

        hadj = GTK_ADJUSTMENT(GLOBALS->wave_hslider);
        gtk_adjustment_set_value(hadj, gt);

        pageinc = (GwTime)(((gdouble)GLOBALS->wavewidth) * GLOBALS->nspx);
        if (gt < (GLOBALS->tims.last - pageinc + 1))
            GLOBALS->tims.timecache = gt;
        else {
            GLOBALS->tims.timecache = GLOBALS->tims.last - pageinc + 1;
            if (GLOBALS->tims.timecache < GLOBALS->tims.first)
                GLOBALS->tims.timecache = GLOBALS->tims.first;
        }

        reformat_time(timval,
                      GLOBALS->tims.timecache + GLOBALS->global_time_offset,
                      GLOBALS->time_dimension);
        sprintf(update_string, "Moved to time: %s\n", timval);
        status_text(update_string);

        time_update();
    }
}

void menu_movetotime(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    char gt[32];

    reformat_time(gt, GLOBALS->tims.start + GLOBALS->global_time_offset, GLOBALS->time_dimension);

    entrybox("Move To Time", 200, gt, NULL, 20, G_CALLBACK(movetotime_cleanup));
}
/**/
static void fetchsize_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    if (GLOBALS->entrybox_text) {
        GwTime fw;
        char update_string[128];
        fw = unformat_time(GLOBALS->entrybox_text, GLOBALS->time_dimension);
        if (fw < 1) {
            fw = GLOBALS->fetchwindow; /* in case they try to pull 0 or <0 */
        } else {
            GLOBALS->fetchwindow = fw;
        }
        free_2(GLOBALS->entrybox_text);
        GLOBALS->entrybox_text = NULL;
        sprintf(update_string, "Fetch Size is now: %" GW_TIME_FORMAT "\n", fw);
        status_text(update_string);
    }
}

void menu_fetchsize(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    char fw[32];

    reformat_time(fw, GLOBALS->fetchwindow, GLOBALS->time_dimension);

    entrybox("New Fetch Size", 200, fw, NULL, 20, G_CALLBACK(fetchsize_cleanup));
}
/**/
void zoomsize_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    if (GLOBALS->entrybox_text) {
        float f;
        char update_string[128];

        sscanf(GLOBALS->entrybox_text, "%f", &f);
        if (f > 0.0) {
            f = 0.0; /* in case they try to go out of range */
        } else if (f < -62.0) {
            f = -62.0; /* in case they try to go out of range */
        }

        GLOBALS->tims.prevzoom = GLOBALS->tims.zoom;
        GLOBALS->tims.zoom = (gdouble)f;
        calczoom(GLOBALS->tims.zoom);
        fix_wavehadj();

        g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "changed");
        g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "value_changed");

        free_2(GLOBALS->entrybox_text);
        GLOBALS->entrybox_text = NULL;
        sprintf(update_string, "Zoom Amount is now: %g\n", f);
        status_text(update_string);
    }
}

void menu_zoomsize(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    char za[32];

    sprintf(za, "%g", (float)(GLOBALS->tims.zoom));

    entrybox("New Zoom Amount", 200, za, NULL, 20, G_CALLBACK(zoomsize_cleanup));
}
/**/
static void zoombase_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    if (GLOBALS->entrybox_text) {
        float za;
        char update_string[128];
        sscanf(GLOBALS->entrybox_text, "%f", &za);
        if (za > 10.0) {
            za = 10.0;
        } else if (za < 1.5) {
            za = 1.5;
        }

        GLOBALS->zoombase = (gdouble)za;
        calczoom(GLOBALS->tims.zoom);
        fix_wavehadj();

        g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "changed");
        g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "value_changed");

        free_2(GLOBALS->entrybox_text);
        GLOBALS->entrybox_text = NULL;
        sprintf(update_string, "Zoom Base is now: %g\n", za);
        status_text(update_string);
    }
}

void menu_zoombase(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    char za[32];

    sprintf(za, "%g", GLOBALS->zoombase);

    entrybox("New Zoom Base Amount", 200, za, NULL, 20, G_CALLBACK(zoombase_cleanup));
}
/**/
static void colorformat(int color)
{
    GwTrace *t;
    int fix = 0;
    int color_prev = WAVE_COLOR_NORMAL;
    int is_first = 0;

    if ((t = GLOBALS->traces.first)) {
        while (t) {
            if (IsSelected(t) && !IsShadowed(t)) {
                if (color != WAVE_COLOR_CYCLE) {
                    t->t_color = color;
                } else {
                    if (!is_first) {
                        is_first = 1;
                        if (t->t_color == WAVE_COLOR_NORMAL) {
                            color_prev = WAVE_COLOR_RED;
                        } else {
                            color_prev = t->t_color;
                        }
                    } else {
                        color_prev++;
                    }

                    if (color_prev > WAVE_COLOR_VIOLET)
                        color_prev = WAVE_COLOR_RED;
                    t->t_color = color_prev;
                }
                fix = 1;
            }
            t = t->t_next;
        }
        if (fix) {
            GLOBALS->signalwindow_width_dirty = 1;
            redraw_signals_and_waves();
        }
    }
}

void menu_colorformat_0(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    colorformat(WAVE_COLOR_NORMAL);
}

void menu_colorformat_1(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    colorformat(WAVE_COLOR_RED);
}

void menu_colorformat_2(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    colorformat(WAVE_COLOR_ORANGE);
}

void menu_colorformat_3(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    colorformat(WAVE_COLOR_YELLOW);
}

void menu_colorformat_4(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    colorformat(WAVE_COLOR_GREEN);
}

void menu_colorformat_5(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    colorformat(WAVE_COLOR_BLUE);
}

void menu_colorformat_6(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    colorformat(WAVE_COLOR_INDIGO);
}

void menu_colorformat_7(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    colorformat(WAVE_COLOR_VIOLET);
}

void menu_colorformat_cyc(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    colorformat(WAVE_COLOR_CYCLE);
}
/**/

char **grow_array(char ***src, int *siz, char *str)
{
    if (!*src) {
        *src = malloc_2(sizeof(char *));
        (*src)[0] = str;
        *siz = 1;
    } else {
        *src = realloc_2(*src, (*siz + 1) * sizeof(char *));
        (*src)[*siz] = str;
        *siz = *siz + 1;
    }

    return (*src);
}

static const gchar *EDITORS_WITH_LINE_NUMBER_SUPPORT[] =
    {"vi", "vim", "gvim", "emacs", "gedit", "code", "codium", NULL};

static void open_index_in_forked_editor(uint32_t idx, int typ)
{
    if (idx == 0) {
        simplereqbox("Open Source", 400, "Source stem not present!", "OK", NULL, NULL, 1);
        return;
    }

    GwStem stem;
    if (typ == FST_MT_SOURCESTEM) {
        stem = gw_stems_get_stem(GLOBALS->stems, idx);
    } else {
        stem = gw_stems_get_istem(GLOBALS->stems, idx);
    }

    char *path = g_strdup(stem.path);

    if (!g_file_test(stem.path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
        char *rp =
            get_relative_adjusted_name(GLOBALS->loaded_file_name, path, GLOBALS->loaded_file_name);
        if (!rp) {
            int clen = strlen(stem.path);
            int wid = MIN(clen * 10, 400);

            simplereqbox("Could not open file!", wid, stem.path, "OK", NULL, NULL, 1);
            return;
        }

        g_free(path);
        path = g_strdup(rp);
        free_2(rp);
    }

    const gchar *editor_name = GLOBALS->editor_name;
    if (editor_name == NULL) {
        editor_name = g_getenv("GTKWAVE_EDITOR");
    }

    if (editor_name != NULL) {
        gchar *editor_command;
        if (strstr(editor_name, "%s") != NULL) {
            char *lineno_str = g_strdup_printf("%d", stem.line_number);

            GRegex *d_re = g_regex_new("%d", 0, 0, NULL);
            GRegex *s_re = g_regex_new("%s", 0, 0, NULL);
            g_assert_nonnull(d_re);
            g_assert_nonnull(s_re);

            gchar *t = g_regex_replace_literal(d_re, editor_name, -1, 0, lineno_str, 0, NULL);
            editor_command = g_regex_replace_literal(s_re, t, -1, 0, path, 0, NULL);
            g_free(t);
        } else if (g_strv_contains(EDITORS_WITH_LINE_NUMBER_SUPPORT, editor_name)) {
            editor_command = g_strdup_printf("%s %s +%d", editor_name, path, stem.line_number);
        } else {
            editor_command = g_strdup_printf("%s %s", editor_name, path);
        }

        GdkDisplay *display = gtk_widget_get_display(GLOBALS->mainwindow);
        GdkAppLaunchContext *app_launch_context = gdk_display_get_app_launch_context(display);
        gdk_app_launch_context_set_timestamp(app_launch_context, GDK_CURRENT_TIME);

        GAppInfoCreateFlags flags = G_APP_INFO_CREATE_NONE;
        if (GLOBALS->editor_run_in_terminal) {
            flags |= G_APP_INFO_CREATE_NEEDS_TERMINAL;
        }

        GAppInfo *app_info = g_app_info_create_from_commandline(editor_command, NULL, flags, NULL);
        g_assert_nonnull(app_info);
        if (!g_app_info_launch(app_info, NULL, G_APP_LAUNCH_CONTEXT(app_launch_context), NULL)) {
            simplereqbox("Could not launch editor!", 400, editor_command, "OK", NULL, NULL, 1);
        }

        g_object_unref(app_info);
        g_object_unref(app_launch_context);
    } else {
        GFile *file = g_file_new_for_path(path);
        gchar *uri = g_file_get_uri(file);

        if (!gtk_show_uri_on_window(GTK_WINDOW(GLOBALS->mainwindow), uri, GDK_CURRENT_TIME, NULL)) {
            simplereqbox("Could not launch default editor!", 400, path, "OK", NULL, NULL, 1);
        }

        g_free(uri);
        g_object_unref(file);
    }

    g_free(path);
}

static void menu_open_hierarchy_2(gpointer null_data,
                                  guint callback_action,
                                  GtkWidget *widget,
                                  int typ)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwTrace *t;
    int fix = 0;
    GwTree *t_forced = NULL;

    if ((t = GLOBALS->traces.first)) {
        while (t) {
            if (IsSelected(t) && !IsShadowed(t)) {
                char *tname = NULL;

                if (!HasWave(t)) {
                    break;
                }

                if (HasAlias(t)) {
                    tname = strdup_2(t->name_full);
                } else if (t->vector == TRUE) {
                    tname = strdup_2(t->n.vec->bvname);
                } else {
                    int flagged = HIER_DEPACK_ALLOC;

                    if (!t->n.nd) {
                        break; /* additional guard on top of !HasWave(t) */
                    }

                    tname = hier_decompress_flagged(t->n.nd->nname, &flagged);
                    if (!flagged) {
                        tname = strdup_2(tname);
                    }
                }

                if (tname) {
                    char *lasthier = strrchr(tname, GLOBALS->hier_delimeter);
                    if (lasthier) {
                        char *tname_copy;

                        lasthier++; /* zero out character after hierarchy */
                        *lasthier = 0;
                        tname_copy = strdup_2(tname); /* force_open_tree_node() is destructive */
                        if (force_open_tree_node(tname_copy, 1, &t_forced) >= 0) {
                            if (GLOBALS->selected_hierarchy_name) {
                                free_2(GLOBALS->selected_hierarchy_name);
                                GLOBALS->selected_hierarchy_name = strdup_2(tname);
                            }

                            select_tree_node(tname);
                        }
                        free_2(tname_copy);
                    }

                    free_2(tname);
                    fix = 1;
                    break;
                }
            }

            t = t->t_next;
        }

        if (fix) {
            GLOBALS->signalwindow_width_dirty = 1;
            redraw_signals_and_waves();
        }
    }

    if (((typ == FST_MT_SOURCESTEM) || (typ == FST_MT_SOURCEISTEM)) && t_forced) {
        uint32_t idx = (typ == FST_MT_SOURCESTEM) ? t_forced->t_stem : t_forced->t_istem;

        if (GLOBALS->stems == NULL || gw_stems_is_empty(GLOBALS->stems)) {
            fprintf(stderr, "GTKWAVE | Could not find stems information in this file!\n");
        } else {
            if (!idx && (typ == FST_MT_SOURCEISTEM) && gw_stems_has_istems(GLOBALS->stems)) {
                /* handle top level where istem == stem and istem is deliberately not specified */
                typ = FST_MT_SOURCESTEM;
                idx = t_forced->t_stem;
            }

            open_index_in_forked_editor(idx, typ);
        }
    }
}

static void menu_open_hierarchy_2a(gpointer null_data,
                                   guint callback_action,
                                   GtkWidget *widget,
                                   int typ)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if ((typ == FST_MT_SOURCESTEM) || (typ == FST_MT_SOURCEISTEM)) {
        GwTree *t_forced = GLOBALS->sst_sig_root_treesearch_gtk2_c_1;

        if (t_forced) {
            uint32_t idx = (typ == FST_MT_SOURCESTEM) ? t_forced->t_stem : t_forced->t_istem;

            if (GLOBALS->stems == NULL || gw_stems_is_empty(GLOBALS->stems)) {
                fprintf(stderr, "GTKWAVE | Could not find stems information in this file!\n");
            } else {
                if (!idx && (typ == FST_MT_SOURCEISTEM) && gw_stems_has_istems(GLOBALS->stems)) {
                    /* handle top level where istem == stem and istem is deliberately not specified
                     */
                    typ = FST_MT_SOURCESTEM;
                    idx = t_forced->t_stem;
                }

                open_index_in_forked_editor(idx, typ);
            }
        }
    }
}

void menu_open_hierarchy(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    menu_open_hierarchy_2(null_data,
                          callback_action,
                          widget,
                          FST_MT_MIN); /* zero for regular open */
}

void menu_open_hierarchy_source(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    menu_open_hierarchy_2(null_data,
                          callback_action,
                          widget,
                          FST_MT_SOURCESTEM); /* for definition source */
}

void menu_open_hierarchy_isource(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    menu_open_hierarchy_2(null_data,
                          callback_action,
                          widget,
                          FST_MT_SOURCEISTEM); /* for instantiation source */
}

void menu_open_sst_hierarchy_source(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    menu_open_hierarchy_2a(null_data,
                           callback_action,
                           widget,
                           FST_MT_SOURCESTEM); /* for definition source */
}

void menu_open_sst_hierarchy_isource(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    menu_open_hierarchy_2a(null_data,
                           callback_action,
                           widget,
                           FST_MT_SOURCEISTEM); /* for instantiation source */
}

/**/

void menu_recurse_import(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;

    recurse_import(widget, callback_action);
}

/**/

static void dataformat(TraceFlagsType mask, TraceFlagsType patch)
{
    GwTrace *t;
    int fix = 0;

    if ((t = GLOBALS->traces.first)) {
        while (t) {
            if (IsSelected(t) && !IsShadowed(t)) {
                t->minmax_valid = 0; /* force analog traces to regenerate if necessary */
                t->flags = ((t->flags) & mask) | patch;
                fix = 1;
            }
            t = t->t_next;
        }
        if (fix) {
            GLOBALS->signalwindow_width_dirty = 1;
            redraw_signals_and_waves();
        }
    }
}

void menu_dataformat_ascii(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_NUMMASK | TR_ANALOGMASK), TR_ASCII);
}

void menu_dataformat_real(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_NUMMASK), TR_REAL);
}

void menu_dataformat_real2bon(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_REAL2BITS | TR_NUMMASK | TR_ANALOGMASK), TR_REAL2BITS | TR_HEX);
}

void menu_dataformat_real2boff(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_REAL2BITS | TR_ANALOGMASK), 0);
}

void menu_dataformat_hex(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_NUMMASK), TR_HEX);
}

void menu_dataformat_dec(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_NUMMASK), TR_DEC);
}

void menu_dataformat_signed(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_NUMMASK), TR_SIGNED);
}

void menu_dataformat_bin(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_NUMMASK), TR_BIN);
}

void menu_dataformat_oct(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_NUMMASK), TR_OCT);
}

void menu_dataformat_rjustify_on(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_RJUSTIFY), TR_RJUSTIFY);
}

void menu_dataformat_rjustify_off(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_RJUSTIFY), 0);
}

void menu_dataformat_bingray_on(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_GRAYMASK | TR_ANALOGMASK), TR_BINGRAY);
}

void menu_dataformat_graybin_on(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_GRAYMASK | TR_ANALOGMASK), TR_GRAYBIN);
}

void menu_dataformat_nogray(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_GRAYMASK | TR_ANALOGMASK), 0);
}

void menu_dataformat_popcnt_on(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_POPCNT), TR_POPCNT);
}

void menu_dataformat_popcnt_off(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_POPCNT), 0);
}

void menu_dataformat_ffo_on(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_FFO), TR_FFO);
}

void menu_dataformat_ffo_off(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_FFO), 0);
}

void menu_dataformat_time(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_NUMMASK), (TR_TIME | TR_DEC));
}

void menu_dataformat_enum(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_NUMMASK), (TR_ENUM | TR_BIN));
}

void menu_dataformat_fpshift_on(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_FPDECSHIFT), TR_FPDECSHIFT);
}

void menu_dataformat_fpshift_off(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_FPDECSHIFT), 0);
}

static void menu_dataformat_fpshift_specify_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    GwTrace *t;
    int fix = 0;
    int shamt = GLOBALS->entrybox_text ? atoi(GLOBALS->entrybox_text) : 0;
    TraceFlagsType mask = ~(TR_FPDECSHIFT);
    TraceFlagsType patch = TR_FPDECSHIFT;

    if ((shamt < 0) || (shamt > 255)) {
        shamt = 0;
        patch = 0;
    }

    if ((t = GLOBALS->traces.first)) {
        while (t) {
            if (IsSelected(t) && !IsShadowed(t)) {
                t->minmax_valid = 0; /* force analog traces to regenerate if necessary */

                t->t_fpdecshift = shamt;
                t->flags = ((t->flags) & mask) | patch;
                fix = 1;
            }
            t = t->t_next;
        }
        if (fix) {
            GLOBALS->signalwindow_width_dirty = 1;
            redraw_signals_and_waves();
        }
    }

    if (GLOBALS->entrybox_text) {
        free_2(GLOBALS->entrybox_text);
        GLOBALS->entrybox_text = NULL;
    }
    GLOBALS->signalwindow_width_dirty = 1;
    redraw_signals_and_waves();
}

void menu_dataformat_fpshift_specify(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    entrybox("Fixed Point Shift Specify",
             300,
             "",
             NULL,
             128,
             G_CALLBACK(menu_dataformat_fpshift_specify_cleanup));

    dataformat(~(TR_FPDECSHIFT), 0);
}

void menu_dataformat_invert_on(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_INVERT), TR_INVERT);
}

void menu_dataformat_invert_off(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_INVERT), 0);
}

void menu_dataformat_reverse_on(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_REVERSE), TR_REVERSE);
}

void menu_dataformat_reverse_off(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_REVERSE), 0);
}

void menu_dataformat_exclude_on(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_EXCLUDE), TR_EXCLUDE);
}

void menu_dataformat_exclude_off(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_EXCLUDE), 0);
}
/**/
void menu_dataformat_rangefill_zero(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_ZEROFILL | TR_ONEFILL | TR_ANALOGMASK), TR_ZEROFILL);
}

void menu_dataformat_rangefill_one(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_ZEROFILL | TR_ONEFILL | TR_ANALOGMASK), TR_ONEFILL);
}

void menu_dataformat_rangefill_off(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_ZEROFILL | TR_ONEFILL | TR_ANALOGMASK), 0);
}
/**/
void menu_dataformat_analog_off(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_ANALOGMASK), 0);
}

void menu_dataformat_analog_step(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_ANALOGMASK), TR_ANALOG_STEP);
}

void menu_dataformat_analog_interpol(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_ANALOGMASK), TR_ANALOG_INTERPOLATED);
}

void menu_dataformat_analog_interpol_step(gpointer null_data,
                                          guint callback_action,
                                          GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_ANALOGMASK), (TR_ANALOG_INTERPOLATED | TR_ANALOG_STEP));
}

void menu_dataformat_analog_resize_screen(gpointer null_data,
                                          guint callback_action,
                                          GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_ANALOG_FULLSCALE), 0);
}

void menu_dataformat_analog_resize_all(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    dataformat(~(TR_ANALOG_FULLSCALE), (TR_ANALOG_FULLSCALE));
}
/**/
void menu_dataformat_highlight_all(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwTrace *t;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    if ((t = GLOBALS->traces.first)) {
        while (t) {
            t->flags |= TR_HIGHLIGHT;
            t = t->t_next;
        }
        redraw_signals_and_waves();
    }
}

void menu_dataformat_unhighlight_all(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwTrace *t;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    if ((t = GLOBALS->traces.first)) {
        while (t) {
            t->flags &= (~TR_HIGHLIGHT);
            t = t->t_next;
        }
        redraw_signals_and_waves();
    }
}

void menu_lexize(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    if (GLOBALS->traces.first) {
        if (TracesReorder(TR_SORT_LEX)) {
            redraw_signals_and_waves();
        }
    }
}
/**/
void menu_alphabetize(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    if (GLOBALS->traces.first) {
        if (TracesReorder(TR_SORT_NORM)) {
            redraw_signals_and_waves();
        }
    }
}
/**/
void menu_alphabetize2(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    if (GLOBALS->traces.first) {
        if (TracesReorder(TR_SORT_INS)) {
            redraw_signals_and_waves();
        }
    }
}
/**/
void menu_reverse(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    if (GLOBALS->traces.first) {
        if (TracesReorder(TR_SORT_RVS)) {
            redraw_signals_and_waves();
        }
    }
}
/**/
void menu_cut_traces(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwTrace *cutbuffer = NULL;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    DEBUG(printf("Cut Traces\n"));

    cutbuffer = CutBuffer();
    if (cutbuffer) {
        if (GLOBALS->cutcopylist) {
            free_2(GLOBALS->cutcopylist);
        }
        GLOBALS->cutcopylist =
            emit_gtkwave_savefile_formatted_entries_in_tcl_list(cutbuffer, FALSE);
        /* printf("Cutlist: '%s'\n", GLOBALS->cutcopylist); */

        redraw_signals_and_waves();
    } else {
        must_sel();
    }
}

void menu_delete_traces(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    int num_cut;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    DEBUG(printf("Delete Traces\n"));

    num_cut = DeleteBuffer();
    if (num_cut) {
        redraw_signals_and_waves();
    } else {
        must_sel();
    }
}

void menu_copy_traces(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    GwTrace *t = GLOBALS->traces.first;
    gboolean highlighted = FALSE;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    DEBUG(printf("Copy Traces\n"));

    while (t) {
        if (t->flags & TR_HIGHLIGHT) {
            highlighted = TRUE;
            break;
        }
        t = t->t_next;
    }

    if (!highlighted) {
        must_sel();
    } else {
        if (GLOBALS->cutcopylist) {
            free_2(GLOBALS->cutcopylist);
        }
        GLOBALS->cutcopylist =
            emit_gtkwave_savefile_formatted_entries_in_tcl_list(GLOBALS->traces.first, TRUE);
        /* printf("Copylist: '%s'\n", GLOBALS->cutcopylist); */

        FreeCutBuffer();
    }
}

void menu_paste_traces(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->dnd_state) {
        dnd_error();
        return;
    } /* don't mess with sigs when dnd active */

    DEBUG(printf("Paste Traces\n"));

    if (PasteBuffer()) {
        GLOBALS->signalwindow_width_dirty = 1;
        redraw_signals_and_waves();
    } else {
        if (GLOBALS->cutcopylist) {
            /*int num_traces =*/process_tcl_list(GLOBALS->cutcopylist, FALSE);
            /* printf("Pastelist: %d '%s'\n", num_traces, GLOBALS->cutcopylist); */

            GLOBALS->signalwindow_width_dirty = 1;
            redraw_signals_and_waves();
        }
    }
}
/**/
void menu_center_zooms(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VCZ]),
            GLOBALS->do_zoom_center =
                GLOBALS->wave_script_args
                    ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                    : (!GLOBALS->do_zoom_center));
    } else {
        GLOBALS->do_zoom_center =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VCZ]));
        DEBUG(printf("Center Zooms\n"));
    }
}

void menu_show_base(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VSBS]),
            GLOBALS->show_base = GLOBALS->wave_script_args
                                     ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                                     : (!GLOBALS->show_base));
    } else {
        GLOBALS->show_base =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VSBS]));
        GLOBALS->signalwindow_width_dirty = 1;
        redraw_signals_and_waves();
        DEBUG(printf("Show Base Symbols\n"));
    }
}

/**/
void menu_fullscreen(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_FULLSCR]),
            GLOBALS->fullscreen = GLOBALS->wave_script_args
                                      ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                                      : (!GLOBALS->fullscreen));
    } else {
        GLOBALS->fullscreen =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_FULLSCR]));
        if (GLOBALS->fullscreen) {
            gtk_window_fullscreen(GTK_WINDOW(GLOBALS->mainwindow));
        } else {
            gtk_window_unfullscreen(GTK_WINDOW(GLOBALS->mainwindow));
        }

        if (GLOBALS->wave_hslider) {
            g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "changed");
            g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "value_changed");
        }
        DEBUG(printf("Fullscreen\n"));
    }
}

void service_fullscreen(GtkWidget *text, gpointer data)
{
    (void)text;
    (void)data;

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_FULLSCR]), TRUE);

    menu_fullscreen(NULL, 0, NULL);
}

void menu_show_grid(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VSG]),
            GLOBALS->display_grid =
                GLOBALS->wave_script_args
                    ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                    : (!GLOBALS->display_grid));
    } else {
        GLOBALS->display_grid =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VSG]));
        if (GLOBALS->wave_hslider) {
            g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "changed");
            g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "value_changed");
        }
        DEBUG(printf("Show Grid\n"));
    }
}

/**/
void menu_show_wave_highlight(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_SHW]),
            GLOBALS->highlight_wavewindow =
                GLOBALS->wave_script_args
                    ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                    : (!GLOBALS->highlight_wavewindow));
    } else {
        GLOBALS->highlight_wavewindow =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_SHW]));
        if (GLOBALS->wave_hslider) {
            g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "changed");
            g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "value_changed");
        }
        DEBUG(printf("Show Wave Highlight\n"));
    }
}

/**/
void menu_show_filled_high_values(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_FILL1]),
            GLOBALS->fill_waveform =
                GLOBALS->wave_script_args
                    ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                    : (!GLOBALS->fill_waveform));
    } else {
        GLOBALS->fill_waveform =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_FILL1]));
        if (GLOBALS->wave_hslider) {
            g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "changed");
            g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "value_changed");
        }
        DEBUG(printf("Show Filled High Values\n"));
    }
}

/**/
void menu_lz_removal(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_LZ_REMOVAL]),
            GLOBALS->lz_removal = GLOBALS->wave_script_args
                                      ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                                      : (!GLOBALS->lz_removal));
    } else {
        GLOBALS->lz_removal =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_LZ_REMOVAL]));
        if (GLOBALS->signalarea && GLOBALS->wavearea) {
            GLOBALS->signalwindow_width_dirty = 1;
            redraw_signals_and_waves();
        }
        DEBUG(printf("Leading Zero Removal\n"));
    }
}

/**/
void menu_show_mouseover(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VSMO]),
            !(GLOBALS->disable_mouseover =
                  GLOBALS->wave_script_args
                      ? (atoi_64(GLOBALS->wave_script_args->payload) ? FALSE : TRUE)
                      : (!GLOBALS->disable_mouseover)));
    } else {
        GLOBALS->disable_mouseover =
            !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VSMO]));
        DEBUG(printf("Show Mouseover\n"));
    }
}

/**/
void menu_clipboard_mouseover(gpointer null_data, guint callback_action, GtkWidget *widget)
{
    (void)null_data;
    (void)callback_action;
    (void)widget;

    if (GLOBALS->tcl_menu_toggle_item) {
        GLOBALS->tcl_menu_toggle_item = FALSE; /* to avoid retriggers */
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VSMC]),
            GLOBALS->clipboard_mouseover =
                GLOBALS->wave_script_args
                    ? (atoi_64(GLOBALS->wave_script_args->payload) ? TRUE : FALSE)
                    : (!GLOBALS->clipboard_mouseover));
    } else {
        GLOBALS->clipboard_mouseover =
            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VSMC]));
        DEBUG(printf("Mouseover Copies To Clipboard\n"));
    }
}
/**/

/* this is the GtkMenuEntry structure used to create new menus.  The
 * first member is the menu definition string.  The second, the
 * default accelerator key used to access this menu function with
 * the keyboard.  The third is the callback function to call when
 * this menu item is selected (by the accelerator key, or with the
 * mouse.) The last member is the data to pass to your callback function.
 *
 * ...This has all been changed to use itemfactory stuff which is more
 * powerful.  The only real difference is the final item which tells
 * the itemfactory just what the item "is".
 */
#ifdef WAVE_USE_MENU_BLACKOUTS
static const char *menu_blackouts[] = {"/Edit", "/Search", "/Time", "/Markers", "/View"};
#endif

static gtkwave_mlist_t menu_items[] = {
    WAVE_GTKIFE("/File/Open New Window", "<Control>N", menu_new_viewer, WV_MENU_FONV, "<Item>"),
    WAVE_GTKIFE("/File/Open New Tab", "<Control>T", menu_new_viewer_tab, WV_MENU_FONVT, "<Item>"),
    WAVE_GTKIFE("/File/Reload Waveform",
                "<Shift><Control>R",
                menu_reload_waveform,
                WV_MENU_FRW,
                "<Item>"),
    WAVE_GTKIFE("/File/Export/Write VCD File As",
                NULL,
                menu_write_vcd_file,
                WV_MENU_WRVCD,
                "<Item>"),
    WAVE_GTKIFE("/File/Export/Write TIM File As",
                NULL,
                menu_write_tim_file,
                WV_MENU_WRTIM,
                "<Item>"),
    WAVE_GTKIFE("/File/Close", "<Control>W", menu_quit_close, WV_MENU_WCLOSE, "<Item>"),
    WAVE_GTKIFE("/File/<separator>", NULL, NULL, WV_MENU_SEP2VCD, "<Separator>"),
    WAVE_GTKIFE("/File/Print To File", "<Control>P", menu_print, WV_MENU_FPTF, "<Item>"),

    WAVE_GTKIFE("/File/Grab To File", NULL, menu_write_screengrab_as, WV_MENU_SGRAB, "<Item>"),

    WAVE_GTKIFE("/File/<separator>", NULL, NULL, WV_MENU_SEP1, "<Separator>"),
    WAVE_GTKIFE("/File/Read Save File", "<Control>O", menu_read_save_file, WV_MENU_FRSF, "<Item>"),
    WAVE_GTKIFE("/File/Write Save File",
                "<Control>S",
                menu_write_save_file,
                WV_MENU_FWSF,
                "<Item>"),
    WAVE_GTKIFE("/File/Write Save File As",
                "<Shift><Control>S",
                menu_write_save_file_as,
                WV_MENU_FWSFAS,
                "<Item>"),
    WAVE_GTKIFE("/File/<separator>", NULL, NULL, WV_MENU_SEP2, "<Separator>"),
    WAVE_GTKIFE("/File/Read Sim Logfile", "L", menu_read_log_file, WV_MENU_FRLF, "<Item>"),
    /* 10 */
    WAVE_GTKIFE("/File/<separator>", NULL, NULL, WV_MENU_SEP2LF, "<Separator>"),
    WAVE_GTKIFE("/File/Read Verilog Stemsfile",
                NULL,
                menu_read_stems_file,
                WV_MENU_FRSTMF,
                "<Item>"),
    WAVE_GTKIFE("/File/<separator>", NULL, NULL, WV_MENU_SEP2STMF, "<Separator>"),
#if defined(HAVE_LIBTCL)
    WAVE_GTKIFE("/File/Read Tcl Script File",
                NULL,
                menu_read_script_file,
                WV_MENU_TCLSCR,
                "<Item>"),
    WAVE_GTKIFE("/File/<separator>", NULL, NULL, WV_MENU_TCLSEP, "<Separator>"),
#endif

    WAVE_GTKIFE("/File/Quit", "<Control>Q", menu_quit, WV_MENU_FQY, "<Item>"),

    WAVE_GTKIFE("/Edit/Set Trace Max Hier", NULL, menu_set_max_hier, WV_MENU_ESTMH, "<Item>"),
    WAVE_GTKIFE("/Edit/Toggle Trace Hier", "H", menu_toggle_hier, WV_MENU_ETH, "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP3, "<Separator>"),
    WAVE_GTKIFE("/Edit/Insert Blank",
                "<Control>B",
                menu_insert_blank_traces,
                WV_MENU_EIB,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Insert Comment", NULL, menu_insert_comment_traces, WV_MENU_EIC, "<Item>"),
    WAVE_GTKIFE("/Edit/Insert Analog Height Extension",
                NULL,
                menu_insert_analog_height_extension,
                WV_MENU_EIA,
                "<Item>"),

#ifdef MAC_INTEGRATION
    WAVE_GTKIFE("/Edit/Cut", NULL, menu_cut_traces, WV_MENU_EC, "<Item>"),
    WAVE_GTKIFE("/Edit/Copy", NULL, menu_copy_traces, WV_MENU_ECY, "<Item>"),
    WAVE_GTKIFE("/Edit/Paste", NULL, menu_paste_traces, WV_MENU_EP, "<Item>"),
    WAVE_GTKIFE("/Edit/Delete", NULL, menu_delete_traces, WV_MENU_DEL, "<Item>"),
#else
    WAVE_GTKIFE("/Edit/Cut", "<Control>X", menu_cut_traces, WV_MENU_EC, "<Item>"),
    WAVE_GTKIFE("/Edit/Copy", "<Control>C", menu_copy_traces, WV_MENU_ECY, "<Item>"),
    WAVE_GTKIFE("/Edit/Paste", "<Control>V", menu_paste_traces, WV_MENU_EP, "<Item>"),
    WAVE_GTKIFE("/Edit/Delete", "<Control>Delete", menu_delete_traces, WV_MENU_DEL, "<Item>"),
#endif

    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP3A, "<Separator>"),

    WAVE_GTKIFE("/Edit/Alias Highlighted Trace", "<Alt>A", menu_alias, WV_MENU_EAHT, "<Item>"),
    WAVE_GTKIFE("/Edit/Remove Highlighted Aliases",
                "<Shift><Alt>A",
                menu_remove_aliases,
                WV_MENU_ERHA,
                "<Item>"),
    /* 20 */
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP4, "<Separator>"),
    WAVE_GTKIFE("/Edit/Expand", "F3", menu_expand, WV_MENU_EE, "<Item>"),
    WAVE_GTKIFE("/Edit/Combine Down", "F4", menu_combine_down, WV_MENU_ECD, "<Item>"),
    WAVE_GTKIFE("/Edit/Combine Up", "F5", menu_combine_up, WV_MENU_ECU, "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP5, "<Separator>"),
    WAVE_GTKIFE("/Edit/Data Format/Hex", "<Alt>X", menu_dataformat_hex, WV_MENU_EDFH, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Decimal", "<Alt>D", menu_dataformat_dec, WV_MENU_EDFD, "<Item>"),
    /* 30 */
    WAVE_GTKIFE("/Edit/Data Format/Signed Decimal",
                NULL,
                menu_dataformat_signed,
                WV_MENU_EDFSD,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Binary", "<Alt>B", menu_dataformat_bin, WV_MENU_EDFB, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Octal", "<Alt>O", menu_dataformat_oct, WV_MENU_EDFO, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/ASCII", NULL, menu_dataformat_ascii, WV_MENU_EDFA, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Time", NULL, menu_dataformat_time, WV_MENU_TIME, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Enum", NULL, menu_dataformat_enum, WV_MENU_ENUM, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/BitsToReal", NULL, menu_dataformat_real, WV_MENU_EDRL, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/RealToBits/On",
                NULL,
                menu_dataformat_real2bon,
                WV_MENU_EDR2BON,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/RealToBits/Off",
                NULL,
                menu_dataformat_real2boff,
                WV_MENU_EDR2BOFF,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Right Justify/On",
                "<Alt>J",
                menu_dataformat_rjustify_on,
                WV_MENU_EDFRJON,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Right Justify/Off",
                "<Shift><Alt>J",
                menu_dataformat_rjustify_off,
                WV_MENU_EDFRJOFF,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Invert/On",
                "<Alt>I",
                menu_dataformat_invert_on,
                WV_MENU_EDFION,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Invert/Off",
                "<Shift><Alt>I",
                menu_dataformat_invert_off,
                WV_MENU_EDFIOFF,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Reverse Bits/On",
                "<Alt>V",
                menu_dataformat_reverse_on,
                WV_MENU_EDFRON,
                "<Item>"),
    /* 40 */
    WAVE_GTKIFE("/Edit/Data Format/Reverse Bits/Off",
                "<Shift><Alt>V",
                menu_dataformat_reverse_off,
                WV_MENU_EDFROFF,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Translate Filter File/Disable",
                NULL,
                menu_dataformat_xlate_file_0,
                WV_MENU_XLF_0,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Translate Filter File/Enable and Select",
                NULL,
                menu_dataformat_xlate_file_1,
                WV_MENU_XLF_1,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Translate Filter Process/Disable",
                NULL,
                menu_dataformat_xlate_proc_0,
                WV_MENU_XLP_0,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Translate Filter Process/Enable and Select",
                NULL,
                menu_dataformat_xlate_proc_1,
                WV_MENU_XLP_1,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Transaction Filter Process/Disable",
                NULL,
                menu_dataformat_xlate_ttrans_0,
                WV_MENU_TTXLP_0,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Transaction Filter Process/Enable and Select",
                NULL,
                menu_dataformat_xlate_ttrans_1,
                WV_MENU_TTXLP_1,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Analog/Off",
                NULL,
                menu_dataformat_analog_off,
                WV_MENU_EDFAOFF,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Analog/Step",
                NULL,
                menu_dataformat_analog_step,
                WV_MENU_EDFASTEP,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Analog/Interpolated",
                NULL,
                menu_dataformat_analog_interpol,
                WV_MENU_EDFAINTERPOL,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Analog/Interpolated Annotated",
                NULL,
                menu_dataformat_analog_interpol_step,
                WV_MENU_EDFAINTERPOL2,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Analog/Resizing/Screen Data",
                NULL,
                menu_dataformat_analog_resize_screen,
                WV_MENU_EDFARSD,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Analog/Resizing/All Data",
                NULL,
                menu_dataformat_analog_resize_all,
                WV_MENU_EDFARAD,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Range Fill/With 0s",
                NULL,
                menu_dataformat_rangefill_zero,
                WV_MENU_RFILL0,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Range Fill/With 1s",
                NULL,
                menu_dataformat_rangefill_one,
                WV_MENU_RFILL1,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Range Fill/Off",
                NULL,
                menu_dataformat_rangefill_off,
                WV_MENU_RFILLOFF,
                "<Item>"),

    WAVE_GTKIFE("/Edit/Data Format/Gray Filters/To Gray",
                NULL,
                menu_dataformat_bingray_on,
                WV_MENU_B2G,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Gray Filters/From Gray",
                NULL,
                menu_dataformat_graybin_on,
                WV_MENU_G2B,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Gray Filters/None",
                NULL,
                menu_dataformat_nogray,
                WV_MENU_GBNONE,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Popcnt/On",
                NULL,
                menu_dataformat_popcnt_on,
                WV_MENU_POPON,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Popcnt/Off",
                NULL,
                menu_dataformat_popcnt_off,
                WV_MENU_POPOFF,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Find First One/On",
                NULL,
                menu_dataformat_ffo_on,
                WV_MENU_FFOON,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Find First One/Off",
                NULL,
                menu_dataformat_ffo_off,
                WV_MENU_FFOOFF,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Fixed Point Shift/On",
                NULL,
                menu_dataformat_fpshift_on,
                WV_MENU_FPSHIFTON,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Fixed Point Shift/Off",
                NULL,
                menu_dataformat_fpshift_off,
                WV_MENU_FPSHIFTOFF,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Fixed Point Shift/Specify",
                NULL,
                menu_dataformat_fpshift_specify,
                WV_MENU_FPSHIFTVAL,
                "<Item>"),

    WAVE_GTKIFE("/Edit/Color Format/Normal", NULL, menu_colorformat_0, WV_MENU_CLRFMT0, "<Item>"),
    WAVE_GTKIFE("/Edit/Color Format/Red", NULL, menu_colorformat_1, WV_MENU_CLRFMT1, "<Item>"),
    WAVE_GTKIFE("/Edit/Color Format/Orange", NULL, menu_colorformat_2, WV_MENU_CLRFMT2, "<Item>"),
    WAVE_GTKIFE("/Edit/Color Format/Yellow", NULL, menu_colorformat_3, WV_MENU_CLRFMT3, "<Item>"),
    WAVE_GTKIFE("/Edit/Color Format/Green", NULL, menu_colorformat_4, WV_MENU_CLRFMT4, "<Item>"),
    WAVE_GTKIFE("/Edit/Color Format/Blue", NULL, menu_colorformat_5, WV_MENU_CLRFMT5, "<Item>"),
    WAVE_GTKIFE("/Edit/Color Format/Indigo", NULL, menu_colorformat_6, WV_MENU_CLRFMT6, "<Item>"),
    WAVE_GTKIFE("/Edit/Color Format/Violet", NULL, menu_colorformat_7, WV_MENU_CLRFMT7, "<Item>"),
    WAVE_GTKIFE("/Edit/Color Format/Cycle", NULL, menu_colorformat_cyc, WV_MENU_CLRFMTC, "<Item>"),
    WAVE_GTKIFE("/Edit/Color Format/<separator>", NULL, NULL, WV_MENU_SEP5A, "<Separator>"),
    WAVE_GTKIFE("/Edit/Color Format/Keep xz Colors",
                NULL,
                menu_keep_xz_colors,
                WV_MENU_KEEPXZ,
                "<ToggleItem>"),
    WAVE_GTKIFE("/Edit/Show-Change All Highlighted",
                NULL,
                menu_showchangeall,
                WV_MENU_ESCAH,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Show-Change First Highlighted",
                "<Control>F",
                menu_showchange,
                WV_MENU_ESCFH,
                "<Item>"),
    /* 50 */
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP6, "<Separator>"),
    WAVE_GTKIFE("/Edit/Time Warp/Warp Marked", NULL, menu_warp_traces, WV_MENU_WARP, "<Item>"),
    WAVE_GTKIFE("/Edit/Time Warp/Unwarp Marked",
                NULL,
                menu_unwarp_traces,
                WV_MENU_UNWARP,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Time Warp/Unwarp All",
                NULL,
                menu_unwarp_traces_all,
                WV_MENU_UNWARPA,
                "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP7A, "<Separator>"),
    WAVE_GTKIFE("/Edit/Exclude",
                "<Shift><Alt>E",
                menu_dataformat_exclude_on,
                WV_MENU_EEX,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Show", "<Shift><Alt>S", menu_dataformat_exclude_off, WV_MENU_ESH, "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP6A, "<Separator>"),
    /* WAVE_GTKIFE("/Edit/Expand All Groups", "F12", menu_expand_all, WV_MENU_EXA, "<Item>"), */
    /* WAVE_GTKIFE("/Edit/Collapse All Groups", "<Shift>F12", menu_collapse_all, WV_MENU_CPA,
       "<Item>"), */
    /* 60 */
    WAVE_GTKIFE("/Edit/Toggle Group Open|Close", "T", menu_toggle_group, WV_MENU_TG, "<Item>"),
    WAVE_GTKIFE("/Edit/Create Group", "G", menu_create_group, WV_MENU_AG, "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP6A1, "<Separator>"),
    WAVE_GTKIFE("/Edit/Highlight Regexp", "<Alt>R", menu_regexp_highlight, WV_MENU_EHR, "<Item>"),
    WAVE_GTKIFE("/Edit/UnHighlight Regexp",
                "<Shift><Alt>R",
                menu_regexp_unhighlight,
                WV_MENU_EUHR,
                "<Item>"),

    WAVE_GTKIFE("/Edit/Highlight All", NULL, menu_dataformat_highlight_all, WV_MENU_EHA, "<Item>"),
    WAVE_GTKIFE("/Edit/UnHighlight All",
                NULL,
                menu_dataformat_unhighlight_all,
                WV_MENU_EUHA,
                "<Item>"),

    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP6B, "<Separator>"),
    WAVE_GTKIFE("/Edit/Sort/Alphabetize All", NULL, menu_alphabetize, WV_MENU_ALPHA, "<Item>"),
    WAVE_GTKIFE("/Edit/Sort/Alphabetize All (CaseIns)",
                NULL,
                menu_alphabetize2,
                WV_MENU_ALPHA2,
                "<Item>"),
    WAVE_GTKIFE("/Edit/Sort/Sigsort All", NULL, menu_lexize, WV_MENU_LEX, "<Item>"),
    WAVE_GTKIFE("/Edit/Sort/Reverse All", NULL, menu_reverse, WV_MENU_RVS, "<Item>"),
    /* 70 */
    WAVE_GTKIFE("/Search/Pattern Search 1", NULL, menu_tracesearchbox, WV_MENU_SPS, "<Item>"),
    WAVE_GTKIFE("/Search/Pattern Search 2", NULL, menu_tracesearchbox, WV_MENU_SPS2, "<Item>"),
    WAVE_GTKIFE("/Search/<separator>", NULL, NULL, WV_MENU_SEP7B, "<Separator>"),
    WAVE_GTKIFE("/Search/Signal Search Regexp", "<Alt>S", menu_signalsearch, WV_MENU_SSR, "<Item>"),
    WAVE_GTKIFE("/Search/<separator>", NULL, NULL, WV_MENU_SEP7, "<Separator>"),
#if !defined __MINGW32__
    WAVE_GTKIFE("/Search/Open Source Definition",
                NULL,
                menu_open_hierarchy_source,
                WV_MENU_OPENHS,
                "<Item>"),
    WAVE_GTKIFE("/Search/Open Source Instantiation",
                NULL,
                menu_open_hierarchy_isource,
                WV_MENU_OPENIHS,
                "<Item>"),
#endif
    WAVE_GTKIFE("/Search/Open Scope", NULL, menu_open_hierarchy, WV_MENU_OPENH, "<Item>"),
    WAVE_GTKIFE("/Search/<separator>", NULL, NULL, WV_MENU_SEP7D, "<Separator>"),
    WAVE_GTKIFE("/Search/Autocoalesce", NULL, menu_autocoalesce, WV_MENU_ACOL, "<ToggleItem>"),
    WAVE_GTKIFE("/Search/Autocoalesce Reversal",
                NULL,
                menu_autocoalesce_reversal,
                WV_MENU_ACOLR,
                "<ToggleItem>"),
    WAVE_GTKIFE("/Search/Autoname Bundles",
                NULL,
                menu_autoname_bundles_on,
                WV_MENU_ABON,
                "<ToggleItem>"),
    /* 80 */
    WAVE_GTKIFE("/Search/<separator>", NULL, NULL, WV_MENU_SEP7C, "<Separator>"),
    WAVE_GTKIFE("/Search/Set Pattern Search Repeat Count",
                NULL,
                menu_strace_repcnt,
                WV_MENU_STRSE,
                "<Item>"),

    WAVE_GTKIFE("/Time/Move To Time", "F1", menu_movetotime, WV_MENU_TMTT, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom Amount", "F2", menu_zoomsize, WV_MENU_TZZA, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom Base", "<Shift>F2", menu_zoombase, WV_MENU_TZZB, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom In",
                "<Control>plus",
                service_zoom_in_marshal,
                WV_MENU_TZZI,
                "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom Out",
                "<Control>minus",
                service_zoom_out_marshal,
                WV_MENU_TZZO,
                "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom Full",
                "<Control>0",
                service_zoom_full_marshal,
                WV_MENU_TZZBFL,
                "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom Best Fit",
                "<Shift><Alt>F",
                service_zoom_fit_marshal,
                WV_MENU_TZZBF,
                "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom To Start",
                "Home",
                service_zoom_left_marshal,
                WV_MENU_TZZTS,
                "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom To End",
                "End",
                service_zoom_right_marshal,
                WV_MENU_TZZTE,
                "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Undo Zoom",
                "<Alt>U",
                service_zoom_undo_marshal,
                WV_MENU_TZUZ,
                "<Item>"),
    /* 90 */
    WAVE_GTKIFE("/Time/Fetch/Fetch Size", "F7", menu_fetchsize, WV_MENU_TFFS, "<Item>"),
    WAVE_GTKIFE("/Time/Fetch/Fetch ->", "<Alt>2", fetch_right_marshal, WV_MENU_TFFR, "<Item>"),
    WAVE_GTKIFE("/Time/Fetch/Fetch <-", "<Alt>1", fetch_left_marshal, WV_MENU_TFFL, "<Item>"),
    WAVE_GTKIFE("/Time/Discard/Discard ->",
                "<Alt>4",
                discard_right_marshal,
                WV_MENU_TDDR,
                "<Item>"),
    WAVE_GTKIFE("/Time/Discard/Discard <-", "<Alt>3", discard_left_marshal, WV_MENU_TDDL, "<Item>"),
    WAVE_GTKIFE("/Time/Shift/Shift ->",
                "<Alt>6",
                service_right_shift_marshal,
                WV_MENU_TSSR,
                "<Item>"),
    WAVE_GTKIFE("/Time/Shift/Shift <-",
                "<Alt>5",
                service_left_shift_marshal,
                WV_MENU_TSSL,
                "<Item>"),
    WAVE_GTKIFE("/Time/Page/Page ->", "<Alt>8", service_right_page_marshal, WV_MENU_TPPR, "<Item>"),
    WAVE_GTKIFE("/Time/Page/Page <-", "<Alt>7", service_left_page_marshal, WV_MENU_TPPL, "<Item>"),
    WAVE_GTKIFE("/Markers/Show-Change Marker Data",
                "<Alt>M",
                menu_markerbox,
                WV_MENU_MSCMD,
                "<Item>"),
    /* 100 */
    WAVE_GTKIFE("/Markers/Drop Named Marker", "<Alt>N", drop_named_marker, WV_MENU_MDNM, "<Item>"),
    WAVE_GTKIFE("/Markers/Collect Named Marker",
                "<Shift><Alt>N",
                collect_named_marker,
                WV_MENU_MCNM,
                "<Item>"),
    WAVE_GTKIFE("/Markers/Collect All Named Markers",
                "<Shift><Control><Alt>N",
                collect_all_named_markers,
                WV_MENU_MCANM,
                "<Item>"),
#ifdef MAC_INTEGRATION
    WAVE_GTKIFE("/Markers/Copy Primary->B Marker", NULL, copy_pri_b_marker, WV_MENU_MCAB, "<Item>"),
#else
    WAVE_GTKIFE("/Markers/Copy Primary->B Marker", "B", copy_pri_b_marker, WV_MENU_MCAB, "<Item>"),
#endif
    WAVE_GTKIFE("/Markers/Delete Primary Marker",
                "<Shift><Alt>M",
                delete_unnamed_marker,
                WV_MENU_MDPM,
                "<Item>"),
    WAVE_GTKIFE("/Markers/<separator>", NULL, NULL, WV_MENU_SEP8, "<Separator>"),
    WAVE_GTKIFE("/Markers/Find Previous Edge",
                NULL,
                service_left_edge_marshal,
                WV_MENU_SLE,
                "<Item>"),
    WAVE_GTKIFE("/Markers/Find Next Edge", NULL, service_right_edge_marshal, WV_MENU_SRE, "<Item>"),
    WAVE_GTKIFE("/Markers/<separator>", NULL, NULL, WV_MENU_SEP8B, "<Separator>"),
    WAVE_GTKIFE("/Markers/Wave Scrolling", NULL, wave_scrolling_on, WV_MENU_MWSON, "<ToggleItem>"),

    WAVE_GTKIFE("/Markers/Locking/Lock to Lesser Named Marker",
                "Q",
                lock_marker_left,
                WV_MENU_MLKLT,
                "<Item>"),
    WAVE_GTKIFE("/Markers/Locking/Lock to Greater Named Marker",
                "W",
                lock_marker_right,
                WV_MENU_MLKRT,
                "<Item>"),
    WAVE_GTKIFE("/Markers/Locking/Unlock from Named Marker",
                "O",
                unlock_marker,
                WV_MENU_MLKOFF,
                "<Item>"),

    WAVE_GTKIFE("/View/Fullscreen", "F11", menu_fullscreen, WV_MENU_FULLSCR, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP8C, "<Separator>"),
    WAVE_GTKIFE("/View/Show Grid", "<Alt>G", menu_show_grid, WV_MENU_VSG, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP9, "<Separator>"),
    WAVE_GTKIFE("/View/Show Wave Highlight",
                NULL,
                menu_show_wave_highlight,
                WV_MENU_SHW,
                "<ToggleItem>"),
    WAVE_GTKIFE("/View/Show Filled High Values",
                NULL,
                menu_show_filled_high_values,
                WV_MENU_FILL1,
                "<ToggleItem>"),
    WAVE_GTKIFE("/View/Leading Zero Removal",
                NULL,
                menu_lz_removal,
                WV_MENU_LZ_REMOVAL,
                "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP9B, "<Separator>"),
    WAVE_GTKIFE("/View/Show Mouseover", NULL, menu_show_mouseover, WV_MENU_VSMO, "<ToggleItem>"),
    WAVE_GTKIFE("/View/Mouseover Copies To Clipboard",
                NULL,
                menu_clipboard_mouseover,
                WV_MENU_VSMC,
                "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP9A, "<Separator>"),
    WAVE_GTKIFE("/View/Show Base Symbols", "<Alt>F1", menu_show_base, WV_MENU_VSBS, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP10, "<Separator>"),
    /* 110 */
    WAVE_GTKIFE("/View/Dynamic Resize",
                "<Alt>9",
                menu_enable_dynamic_resize,
                WV_MENU_VDR,
                "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP11, "<Separator>"),
    WAVE_GTKIFE("/View/Center Zooms", "F8", menu_center_zooms, WV_MENU_VCZ, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP12, "<Separator>"),
    WAVE_GTKIFE("/View/Toggle Delta-Frequency",
                NULL,
                menu_toggle_delta_or_frequency,
                WV_MENU_VTDF,
                "<Item>"),
    WAVE_GTKIFE("/View/Toggle Max-Marker", NULL, menu_toggle_max_or_marker, WV_MENU_VTMM, "<Item>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP13, "<Separator>"),
    WAVE_GTKIFE("/View/Constant Marker Update",
                NULL,
                menu_enable_constant_marker_update,
                WV_MENU_VCMU,
                "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP14, "<Separator>"),
    WAVE_GTKIFE("/View/Draw Roundcapped Vectors",
                "<Alt>F2",
                menu_use_roundcaps,
                WV_MENU_VDRV,
                "<ToggleItem>"),
    /* 120 */
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP15, "<Separator>"),
    WAVE_GTKIFE("/View/Left Justified Signals",
                "<Shift>Home",
                menu_left_justify,
                WV_MENU_VLJS,
                "<Item>"),
    WAVE_GTKIFE("/View/Right Justified Signals",
                "<Shift>End",
                menu_right_justify,
                WV_MENU_VRJS,
                "<Item>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP16, "<Separator>"),
    WAVE_GTKIFE("/View/Zoom Pow10 Snap",
                "<Shift>Pause",
                menu_zoom10_snap,
                WV_MENU_VZPS,
                "<ToggleItem>"),
    WAVE_GTKIFE("/View/Partial VCD Dynamic Zoom Full",
                NULL,
                menu_zoom_dynf,
                WV_MENU_VZDYN,
                "<ToggleItem>"),
    WAVE_GTKIFE("/View/Partial VCD Dynamic Zoom To End",
                NULL,
                menu_zoom_dyne,
                WV_MENU_VZDYNE,
                "<ToggleItem>"),
    WAVE_GTKIFE("/View/Full Precision",
                "<Alt>Pause",
                menu_use_full_precision,
                WV_MENU_VFTP,
                "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP17, "<Separator>"),
    WAVE_GTKIFE("/View/Define Time Ruler Marks", NULL, menu_def_ruler, WV_MENU_RULER, "<Item>"),
    WAVE_GTKIFE("/View/Remove Pattern Marks", NULL, menu_remove_marked, WV_MENU_RMRKS, "<Item>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP17A, "<Separator>"),
    WAVE_GTKIFE("/View/Use Color", NULL, menu_use_color, WV_MENU_USECOLOR, "<Item>"),
    WAVE_GTKIFE("/View/Use Black and White", NULL, menu_use_bw, WV_MENU_USEBW, "<Item>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP18, "<Separator>"),
    WAVE_GTKIFE("/View/Scale To Time Dimension/None",
                NULL,
                menu_scale_to_td_x,
                WV_MENU_TDSCALEX,
                "<ToggleItem>"),
    WAVE_GTKIFE("/View/Scale To Time Dimension/sec",
                NULL,
                menu_scale_to_td_s,
                WV_MENU_TDSCALES,
                "<ToggleItem>"),
    WAVE_GTKIFE("/View/Scale To Time Dimension/ms",
                NULL,
                menu_scale_to_td_m,
                WV_MENU_TDSCALEM,
                "<ToggleItem>"),
    WAVE_GTKIFE("/View/Scale To Time Dimension/us",
                NULL,
                menu_scale_to_td_u,
                WV_MENU_TDSCALEU,
                "<ToggleItem>"),
    WAVE_GTKIFE("/View/Scale To Time Dimension/ns",
                NULL,
                menu_scale_to_td_n,
                WV_MENU_TDSCALEN,
                "<ToggleItem>"),
    WAVE_GTKIFE("/View/Scale To Time Dimension/ps",
                NULL,
                menu_scale_to_td_p,
                WV_MENU_TDSCALEP,
                "<ToggleItem>"),
    WAVE_GTKIFE("/View/Scale To Time Dimension/fs",
                NULL,
                menu_scale_to_td_f,
                WV_MENU_TDSCALEF,
                "<ToggleItem>"),

/* 130 */
#ifdef MAC_INTEGRATION
    WAVE_GTKIFE("/Help/WAVE User Manual", NULL, menu_help_manual, WV_MENU_HWM, "<Item>"),
#endif
    WAVE_GTKIFE("/Help/Wave Version", NULL, menu_version, WV_MENU_HWV, "<Item>"),
};

void set_scale_to_time_dimension_toggles(void)
{
    int i, ii;

    switch (GLOBALS->scale_to_time_dimension) {
        case 's':
            ii = WV_MENU_TDSCALES;
            break;
        case 'm':
            ii = WV_MENU_TDSCALEM;
            break;
        case 'u':
            ii = WV_MENU_TDSCALEU;
            break;
        case 'n':
            ii = WV_MENU_TDSCALEN;
            break;
        case 'p':
            ii = WV_MENU_TDSCALEP;
            break;
        case 'f':
            ii = WV_MENU_TDSCALEF;
            break;
        default:
            ii = WV_MENU_TDSCALEX;
            break;
    }

    for (i = WV_MENU_TDSCALEX; i <= WV_MENU_TDSCALEF; i++) {
        gboolean is_active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_wlist[i]));
        if (i != ii) {
            if (is_active) {
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[i]), FALSE);
            }
        } else {
            if (!is_active) {
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[i]), TRUE);
            }
        }
    }
}

/*
 * set toggleitems to their initial states
 */
void set_menu_toggles(void)
{
    GLOBALS->quiet_checkmenu = 1;

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VZPS]),
                                   GLOBALS->zoom_pow10_snap);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_FULLSCR]),
                                   GLOBALS->fullscreen);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VSG]),
                                   GLOBALS->display_grid);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_SHW]),
                                   GLOBALS->highlight_wavewindow);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_FILL1]),
                                   GLOBALS->fill_waveform);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_LZ_REMOVAL]),
                                   GLOBALS->lz_removal);

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VSMO]),
                                   !GLOBALS->disable_mouseover);

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VSMC]),
                                   GLOBALS->clipboard_mouseover);

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VSBS]),
                                   GLOBALS->show_base);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VDR]),
                                   GLOBALS->do_resize_signals);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VCMU]),
                                   GLOBALS->constant_marker_update);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VCZ]),
                                   GLOBALS->do_zoom_center);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VDRV]),
                                   GLOBALS->use_roundcaps);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_MWSON]),
                                   GLOBALS->wave_scrolling);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_ABON]),
                                   GLOBALS->autoname_bundles);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VFTP]),
                                   GLOBALS->use_full_precision);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_ACOL]),
                                   GLOBALS->autocoalesce);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_ACOLR]),
                                   GLOBALS->autocoalesce_reversal);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_KEEPXZ]),
                                   GLOBALS->keep_xz_colors);

    if (GLOBALS->partial_vcd) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VZDYN]),
                                       GLOBALS->zoom_dyn);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_wlist[WV_MENU_VZDYNE]),
                                       GLOBALS->zoom_dyne);
    }

    set_scale_to_time_dimension_toggles();
    GLOBALS->quiet_checkmenu = 0;
}

/*
 * kill accelerator keys (e.g., if using twinwave as focus is sometimes wrong from parent window)
 */
void kill_main_menu_accelerators(void)
{
    int i;

    for (i = 0; i < WV_MENU_NUMITEMS; i++) {
        menu_items[i].accelerator = NULL;
    }
}

/*
 * create the menu through an itemfactory instance
 */

void menu_set_sensitive(void)
{
    int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
    int i;
    GtkWidget *mw;
#ifdef WAVE_USE_MENU_BLACKOUTS
    for (i = 0; i < (sizeof(menu_blackouts) / sizeof(char *)); i++) {
        mw = gtk_item_factory_get_widget(GLOBALS->item_factory_menu_c_1, menu_blackouts[i]);
        if (mw)
            gtk_widget_set_sensitive(mw, TRUE);
    }
#endif

    for (i = 0; i < nmenu_items; i++) {
        switch (i) {
            case WV_MENU_FONVT:
            case WV_MENU_WCLOSE:
#if defined(HAVE_LIBTCL)
            case WV_MENU_TCLSCR:
#endif
            case WV_MENU_FQY:
#ifdef MAC_INTEGRATION
            case WV_MENU_HWM:
#endif
            case WV_MENU_HWV:
                break;

            default:
                mw = menu_wlist[i];
                if (mw) {
#ifdef MAC_INTEGRATION
                    if (menu_items[i].callback)
#endif
                    {
                        gtk_widget_set_sensitive(mw, TRUE);
                    }
                }
                break;
        }
    }
}

/*
 * bail out
 */
int file_quit_cmd_callback(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    if (GLOBALS->save_on_exit) {
        menu_write_save_file(NULL, 0, NULL);
    }

    if (!GLOBALS->enable_fast_exit) {
        simplereqbox("Quit Program",
                     300,
                     "Do you really want to quit?",
                     "Yes",
                     "No",
                     G_CALLBACK(menu_quit_callback),
                     1);
    } else {
#ifdef __CYGWIN__
        kill_stems_browser();
#endif
        g_print("WM Destroy\n");
        exit(0);
    }

    return (
        TRUE); /* keeps "delete_event" from happening...we'll manually destory later if need be */
}

/*
 * RPC
 */
int execute_script(char *name, int dealloc_name)
{
    unsigned int i;
    int nlen = strlen(name);

    if (GLOBALS->tcl_running) {
        fprintf(stderr, "Could not run script file '%s', as one is already running.\n", name);

        if (dealloc_name) {
            free_2(name);
        }

        return (0);
    }

    GLOBALS->tcl_running = 1;

    if (1) /* all scripts are Tcl now */
    {
#if defined(HAVE_LIBTCL)
        int tclrc;
        char *tcl_cmd = g_alloca(
            8 + nlen + 1 + 1); /* originally a malloc, but the script can change the context! */
        strcpy(tcl_cmd, "source {");
        strcpy(tcl_cmd + 8, name);
        strcpy(tcl_cmd + 8 + nlen, "}");

        fprintf(stderr, "GTKWAVE | Executing Tcl script '%s'\n", name);

        if (dealloc_name) {
            free_2(name);
        }

#ifdef WIN32
        {
            char *slashfix = tcl_cmd;
            while (*slashfix) {
                if (*slashfix == '\\')
                    *slashfix = '/';
                slashfix++;
            }
        }
#endif

        tclrc = Tcl_Eval(GLOBALS->interp, tcl_cmd);
        if (tclrc != TCL_OK) {
            fprintf(stderr, "GTKWAVE | %s\n", Tcl_GetStringResult(GLOBALS->interp));
        }
#else
        fprintf(stderr, "GTKWAVE | Tcl support not compiled into gtkwave, exiting.\n");
        exit(255);
#endif
    }

    for (i = 0; i < GLOBALS->num_notebook_pages; i++) {
        (*GLOBALS->contexts)[i]->wave_script_args = NULL; /* just in case there was a CTX swap */
    }

    GLOBALS->tcl_running = 0;
    return (0);
}

gtkwave_mlist_t *retrieve_menu_items_array(int *num_items)
{
    *num_items = sizeof(menu_items) / sizeof(menu_items[0]);

    return (menu_items);
}

/*
 * support for menu accelerator modifications...
 */
int set_wave_menu_accelerator(const char *str)
{
    char *path, *pathend;
    char *accel;
    int i;

    path = strchr(str, '\"');
    if (!path)
        return (1);
    path++;
    if (!*path)
        return (1);

    pathend = strchr(path, '\"');
    if (!pathend)
        return (1);

    *pathend = 0;

    accel = pathend + 1;
    while (*accel) {
        if (!isspace((int)(unsigned char)*accel))
            break;
        accel++;
    }

    if (!*accel)
        return (1);

    if (strstr(path, "<separator>"))
        return (1);
    if (!strcmp(accel, "(null)")) {
        accel = NULL;
    } else {
        for (i = 0; i < WV_MENU_NUMITEMS; i++) {
            if (menu_items[i].accelerator) {
                if (!strcmp(menu_items[i].accelerator, accel)) {
                    menu_items[i].accelerator = NULL;
                }
            }
        }
    }

    for (i = 0; i < WV_MENU_NUMITEMS; i++) {
        if (menu_items[i].path) {
            if (!strcmp(menu_items[i].path, path)) {
                menu_items[i].accelerator = accel ? strdup_2(accel) : NULL;
                break;
            }
        }
    }

    return (0);
}

/***********************/
/*** popup menu code ***/
/***********************/

static gtkwave_mlist_t popmenu_items[] = {
    WAVE_GTKIFE("/Data Format/Hex", NULL, menu_dataformat_hex, WV_MENU_EDFH, "<Item>"),
    WAVE_GTKIFE("/Data Format/Decimal", NULL, menu_dataformat_dec, WV_MENU_EDFD, "<Item>"),
    WAVE_GTKIFE("/Data Format/Signed Decimal",
                NULL,
                menu_dataformat_signed,
                WV_MENU_EDFSD,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Binary", NULL, menu_dataformat_bin, WV_MENU_EDFB, "<Item>"),
    WAVE_GTKIFE("/Data Format/Octal", NULL, menu_dataformat_oct, WV_MENU_EDFO, "<Item>"),
    WAVE_GTKIFE("/Data Format/ASCII", NULL, menu_dataformat_ascii, WV_MENU_EDFA, "<Item>"),
    WAVE_GTKIFE("/Data Format/Time", NULL, menu_dataformat_time, WV_MENU_TIME, "<Item>"),
    WAVE_GTKIFE("/Data Format/Enum", NULL, menu_dataformat_enum, WV_MENU_ENUM, "<Item>"),
    WAVE_GTKIFE("/Data Format/BitsToReal", NULL, menu_dataformat_real, WV_MENU_EDRL, "<Item>"),
    WAVE_GTKIFE("/Data Format/RealToBits/On",
                NULL,
                menu_dataformat_real2bon,
                WV_MENU_EDR2BON,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/RealToBits/Off",
                NULL,
                menu_dataformat_real2boff,
                WV_MENU_EDR2BOFF,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Right Justify/On",
                NULL,
                menu_dataformat_rjustify_on,
                WV_MENU_EDFRJON,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Right Justify/Off",
                NULL,
                menu_dataformat_rjustify_off,
                WV_MENU_EDFRJOFF,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Invert/On",
                NULL,
                menu_dataformat_invert_on,
                WV_MENU_EDFION,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Invert/Off",
                NULL,
                menu_dataformat_invert_off,
                WV_MENU_EDFIOFF,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Reverse Bits/On",
                NULL,
                menu_dataformat_reverse_on,
                WV_MENU_EDFRON,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Reverse Bits/Off",
                NULL,
                menu_dataformat_reverse_off,
                WV_MENU_EDFROFF,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Translate Filter File/Disable",
                NULL,
                menu_dataformat_xlate_file_0,
                WV_MENU_XLF_0,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Translate Filter File/Enable and Select",
                NULL,
                menu_dataformat_xlate_file_1,
                WV_MENU_XLF_1,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Translate Filter Process/Disable",
                NULL,
                menu_dataformat_xlate_proc_0,
                WV_MENU_XLP_0,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Translate Filter Process/Enable and Select",
                NULL,
                menu_dataformat_xlate_proc_1,
                WV_MENU_XLP_1,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Transaction Filter Process/Disable",
                NULL,
                menu_dataformat_xlate_ttrans_0,
                WV_MENU_TTXLP_0,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Transaction Filter Process/Enable and Select",
                NULL,
                menu_dataformat_xlate_ttrans_1,
                WV_MENU_TTXLP_1,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Analog/Off",
                NULL,
                menu_dataformat_analog_off,
                WV_MENU_EDFAOFF,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Analog/Step",
                NULL,
                menu_dataformat_analog_step,
                WV_MENU_EDFASTEP,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Analog/Interpolated",
                NULL,
                menu_dataformat_analog_interpol,
                WV_MENU_EDFAINTERPOL,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Analog/Interpolated Annotated",
                NULL,
                menu_dataformat_analog_interpol_step,
                WV_MENU_EDFAINTERPOL2,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Analog/Resizing/Screen Data",
                NULL,
                menu_dataformat_analog_resize_screen,
                WV_MENU_EDFARSD,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Analog/Resizing/All Data",
                NULL,
                menu_dataformat_analog_resize_all,
                WV_MENU_EDFARAD,
                "<Item>"),

    WAVE_GTKIFE("/Data Format/Range Fill/With 0s",
                NULL,
                menu_dataformat_rangefill_zero,
                WV_MENU_RFILL0,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Range Fill/With 1s",
                NULL,
                menu_dataformat_rangefill_one,
                WV_MENU_RFILL1,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Range Fill/Off",
                NULL,
                menu_dataformat_rangefill_off,
                WV_MENU_RFILLOFF,
                "<Item>"),

    WAVE_GTKIFE("/Data Format/Gray Filters/To Gray",
                NULL,
                menu_dataformat_bingray_on,
                WV_MENU_B2G,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Gray Filters/From Gray",
                NULL,
                menu_dataformat_graybin_on,
                WV_MENU_G2B,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Gray Filters/None",
                NULL,
                menu_dataformat_nogray,
                WV_MENU_GBNONE,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Popcnt/On", NULL, menu_dataformat_popcnt_on, WV_MENU_POPON, "<Item>"),
    WAVE_GTKIFE("/Data Format/Popcnt/Off",
                NULL,
                menu_dataformat_popcnt_off,
                WV_MENU_POPOFF,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Find First One/On",
                NULL,
                menu_dataformat_ffo_on,
                WV_MENU_FFOON,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Find First One/Off",
                NULL,
                menu_dataformat_ffo_off,
                WV_MENU_FFOOFF,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Fixed Point Shift/On",
                NULL,
                menu_dataformat_fpshift_on,
                WV_MENU_FPSHIFTON,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Fixed Point Shift/Off",
                NULL,
                menu_dataformat_fpshift_off,
                WV_MENU_FPSHIFTOFF,
                "<Item>"),
    WAVE_GTKIFE("/Data Format/Fixed Point Shift/Specify",
                NULL,
                menu_dataformat_fpshift_specify,
                WV_MENU_FPSHIFTVAL,
                "<Item>"),

    WAVE_GTKIFE("/Color Format/Normal", NULL, menu_colorformat_0, WV_MENU_CLRFMT0, "<Item>"),
    WAVE_GTKIFE("/Color Format/Red", NULL, menu_colorformat_1, WV_MENU_CLRFMT1, "<Item>"),
    WAVE_GTKIFE("/Color Format/Orange", NULL, menu_colorformat_2, WV_MENU_CLRFMT2, "<Item>"),
    WAVE_GTKIFE("/Color Format/Yellow", NULL, menu_colorformat_3, WV_MENU_CLRFMT3, "<Item>"),
    WAVE_GTKIFE("/Color Format/Green", NULL, menu_colorformat_4, WV_MENU_CLRFMT4, "<Item>"),
    WAVE_GTKIFE("/Color Format/Blue", NULL, menu_colorformat_5, WV_MENU_CLRFMT5, "<Item>"),
    WAVE_GTKIFE("/Color Format/Indigo", NULL, menu_colorformat_6, WV_MENU_CLRFMT6, "<Item>"),
    WAVE_GTKIFE("/Color Format/Violet", NULL, menu_colorformat_7, WV_MENU_CLRFMT7, "<Item>"),
    WAVE_GTKIFE("/Color Format/Cycle", NULL, menu_colorformat_cyc, WV_MENU_CLRFMTC, "<Item>"),
    WAVE_GTKIFE("/<separator>", NULL, NULL, WV_MENU_SEP1, "<Separator>"),
    WAVE_GTKIFE("/Insert Analog Height Extension",
                NULL,
                menu_insert_analog_height_extension,
                WV_MENU_EIA,
                "<Item>"),
    WAVE_GTKIFE("/<separator>", NULL, NULL, WV_MENU_SEP2, "<Separator>"),

    WAVE_GTKIFE("/Insert Blank", NULL, menu_insert_blank_traces, WV_MENU_EIB, "<Item>"),
    WAVE_GTKIFE("/Insert Comment", NULL, menu_insert_comment_traces, WV_MENU_EIC, "<Item>"),
    WAVE_GTKIFE("/Alias Highlighted Trace", NULL, menu_alias, WV_MENU_EAHT, "<Item>"),
    WAVE_GTKIFE("/Remove Highlighted Aliases", NULL, menu_remove_aliases, WV_MENU_ERHA, "<Item>"),
    WAVE_GTKIFE("/<separator>", NULL, NULL, WV_MENU_SEP3, "<Separator>"),
    WAVE_GTKIFE("/Cut", NULL, menu_cut_traces, WV_MENU_EC, "<Item>"),
    WAVE_GTKIFE("/Copy", NULL, menu_copy_traces, WV_MENU_ECY, "<Item>"),
    WAVE_GTKIFE("/Paste", NULL, menu_paste_traces, WV_MENU_EP, "<Item>"),
    WAVE_GTKIFE("/Delete", NULL, menu_delete_traces, WV_MENU_DEL, "<Item>"),
    WAVE_GTKIFE("/<separator>", NULL, NULL, WV_MENU_SEP4, "<Separator>"),
    WAVE_GTKIFE("/Open Scope", NULL, menu_open_hierarchy, WV_MENU_OPENH, "<Item>"),
#if !defined __MINGW32__
    /* see do_popup_menu() for specific patch for this for if(!GLOBALS->stem_path_string_table) ...
     */
    WAVE_GTKIFE("/Open Source Definition",
                NULL,
                menu_open_hierarchy_source,
                WV_MENU_OPENHS,
                "<Item>"),
    WAVE_GTKIFE("/Open Source Instantiation",
                NULL,
                menu_open_hierarchy_isource,
                WV_MENU_OPENIHS,
                "<Item>"),
#endif
    WAVE_GTKIFE("/<separator>", NULL, NULL, WV_MENU_SEP5, "<Separator>"),
    WAVE_GTKIFE("/Trace Properties", NULL, menu_showchangeall, WV_MENU_ESCFH, "<Item>"),
};

void do_popup_menu(GtkWidget *my_widget, GdkEventButton *event)
{
    (void)my_widget;
    (void)event;

    GtkWidget *menu;
    if (!GLOBALS->signal_popup_menu) {
        int nmenu_items = sizeof(popmenu_items) / sizeof(popmenu_items[0]);

        // TODO: disable menu items instead
        // #if !defined __MINGW32__
        //         if (!GLOBALS->stem_path_string_table) {
        //             nmenu_items = nmenu_items -
        //                           2; /* to remove WV_MENU_OPENHS, WV_MENU_OPENIHS -> keep at end
        //                           of list! */
        //         } else {
        //             if (!GLOBALS->istem_struct_base) {
        //                 nmenu_items--; /* remove "/Open Source Instantiation" if not present */
        //             }
        //         }
        // #endif

        GLOBALS->signal_popup_menu = menu = alt_menu(popmenu_items, nmenu_items, NULL, NULL, FALSE);
    } else {
        menu = GLOBALS->signal_popup_menu;
    }

    gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);
}

/***************************/
/*** sst popup menu code ***/
/***************************/

static gtkwave_mlist_t sst_popmenu_items[] = {
    WAVE_GTKIFE("/Recurse Import/Append", NULL, menu_recurse_import, WV_RECURSE_APPEND, "<Item>"),
    WAVE_GTKIFE("/Recurse Import/Insert", NULL, menu_recurse_import, WV_RECURSE_INSERT, "<Item>"),
    WAVE_GTKIFE("/Recurse Import/Replace", NULL, menu_recurse_import, WV_RECURSE_REPLACE, "<Item>"),

#if !defined __MINGW32__
    WAVE_GTKIFE("/Open Source Definition",
                NULL,
                menu_open_sst_hierarchy_source,
                WV_MENU_OPENHS,
                "<Item>"),
    WAVE_GTKIFE("/Open Source Instantiation",
                NULL,
                menu_open_sst_hierarchy_isource,
                WV_MENU_OPENIHS,
                "<Item>"),
#endif
};

void do_sst_popup_menu(GtkWidget *my_widget, GdkEventButton *event)
{
    (void)my_widget;
    (void)event;

    GtkWidget *menu;
    if (!GLOBALS->sst_signal_popup_menu) {
        int nmenu_items = sizeof(sst_popmenu_items) / sizeof(sst_popmenu_items[0]);

        // TODO: disable menu items instead
        // #if !defined __MINGW32__
        //         if (!GLOBALS->stem_path_string_table) {
        //             nmenu_items -= 2; /* remove all stems popups */
        //         } else if (!GLOBALS->istem_struct_base) {
        //             nmenu_items--; /* remove "/Open Source Instantiation" if not present */
        //         }
        //         /* still have recurse import popup */
        // #endif

        GLOBALS->sst_signal_popup_menu = menu =
            alt_menu(sst_popmenu_items, nmenu_items, NULL, NULL, FALSE);
    } else {
        menu = GLOBALS->sst_signal_popup_menu;
    }

    gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);
}

void SetTraceScrollbarRowValue(int row, unsigned location)
{
    if (row >= 0) {
        int target = row;

        int num_traces_displayable =
            gw_signal_list_get_num_traces_displayable(GW_SIGNAL_LIST(GLOBALS->signalarea));

        /* center */
        if (location == 1) {
            target = target - num_traces_displayable / 2;
        }

        /* end */
        if (location == 2) {
            target = target - num_traces_displayable;
        }

        gw_signal_list_scroll(GW_SIGNAL_LIST(GLOBALS->signalarea), target);

        gtkwave_main_iteration();
    }
}

/*
 * the following is for the eventual migration to GtkMenu from the item factory.
 * all menu features are implemented.
 */

struct menu_item_t
{
    struct menu_item_t *next;
    struct menu_item_t *child;

    char *name;
    int idx;
    unsigned valid : 1;
};

static void decompose_path(const char *pathname, int *items, char ***parts)
{
    char *s, **slashes;
    int i;
    int slashcount = 0;
    char *p_copy = strdup_2(pathname);

    s = p_copy;
    while (*s) {
        if (*s == '/')
            slashcount++;
        s++;
    }

    *parts = calloc_2(slashcount, sizeof(char *));
    slashes = calloc_2(slashcount, sizeof(char *));

    s = p_copy;
    slashcount = 0;
    while (*s) {
        if (*s == '/')
            slashes[slashcount++] = s;
        s++;
    }

    for (i = 0; i < slashcount; i++) {
        if (i != (slashcount - 1))
            *(slashes[i + 1]) = 0;
        (*parts)[i] = strdup_2(slashes[i] + 1);

        if (i != (slashcount - 1))
            *(slashes[i + 1]) = '/';
    }

    *items = slashcount;

    free_2(slashes);
    free_2(p_copy);
}

static void free_decomposed_path(int items, char **parts)
{
    int i;
    for (i = 0; i < items; i++) {
        free_2(parts[i]);
    }

    free_2(parts);
}

static void alt_menu_install_accelerator(GtkAccelGroup *accel,
                                         GtkWidget *menuitem,
                                         const char *accelerator,
                                         const char *path)
{
    if (accel && menuitem && path) {
        int no_map = 0;
        guint accelerator_key = 0;
        GdkModifierType accelerator_mods = 0;
        char full_path[1024];
        sprintf(full_path, "<main>%s", path);

        if (accelerator) {
            gtk_accelerator_parse(accelerator, &accelerator_key, &accelerator_mods);

#ifdef MAC_INTEGRATION
            if ((accelerator_mods & GDK_MOD1_MASK) || (!accelerator_mods) ||
                (accelerator_mods == GDK_SHIFT_MASK)) {
                no_map = 1; /* ALT not available with GTK menus on OSX? */
                /* also remove "normal" keys to avoid conflicts with OSX menubar */
            }
            if (accelerator_mods & GDK_CONTROL_MASK) {
                accelerator_mods &= ~GDK_CONTROL_MASK;
                accelerator_mods |= GDK_META_MASK;
            }
#endif
        }

        if (!no_map) {
            gtk_accel_map_add_entry(full_path, accelerator_key, accelerator_mods);
            gtk_widget_set_accel_path(menuitem, full_path, accel);
        }
    }
}

static GtkWidget *alt_menu_walk(gtkwave_mlist_t *mi,
                                GtkWidget **wlist,
                                struct menu_item_t *lst,
                                int depth,
                                GtkAccelGroup *accel)
{
    struct menu_item_t *ptr = lst;
    struct menu_item_t *optr;
    GtkWidget *menu;
    GtkWidget *menuitem;

    if (depth) {
        menu = gtk_menu_new();

        if (accel)
            gtk_menu_set_accel_group(GTK_MENU(menu), accel);
    } else {
        menu = gtk_menu_bar_new();
    }

    while (ptr) {
        /*  mi[ptr->idx] is menu item */

        if (!strcmp(mi[ptr->idx].item_type, "<Separator>")) {
#if defined(MAC_INTEGRATION) || defined(WAVE_GTK3_MENU_SEPARATOR)
            menuitem = gtk_separator_menu_item_new();
#else
            menuitem = gtk_menu_item_new();
#endif
        } else {
            if ((!strcmp(mi[ptr->idx].item_type, "<ToggleItem>")) && !ptr->child) {
                menuitem = gtk_check_menu_item_new_with_label(ptr->name);
            } else {
                menuitem = gtk_menu_item_new_with_label(ptr->name);
            }

            if (!ptr->child && mi[ptr->idx].callback) {
                g_signal_connect(menuitem,
                                 "activate",
                                 G_CALLBACK(mi[ptr->idx].callback),
                                 (gpointer)(intptr_t)mi[ptr->idx].callback_action);
                alt_menu_install_accelerator(accel,
                                             menuitem,
                                             mi[ptr->idx].accelerator,
                                             mi[ptr->idx].path);
            }
        }

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
        gtk_widget_show(menuitem);

        if (wlist) {
            wlist[ptr->idx] = menuitem;
        }

        if (ptr->child) {
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem),
                                      alt_menu_walk(mi, wlist, ptr->child, depth + 1, accel));
        }

        optr = ptr;
        ptr = ptr->next;

        free_2(optr->name);
        free_2(optr);
    }

    return (menu);
}

GtkWidget *alt_menu(gtkwave_mlist_t *mi,
                    int nmenu_items,
                    GtkWidget **wlist,
                    GtkAccelGroup *accel,
                    gboolean is_menubar)
{
    int i, j;
    struct menu_item_t *mtree = calloc_2(1, sizeof(struct menu_item_t));
    struct menu_item_t *n, *n2 = NULL, *n3;
    GtkWidget *menubar;

    for (i = 0; i < nmenu_items; i++) {
        char **parts;
        int items;

        decompose_path(mi[i].path, &items, &parts);

        n = mtree;
        for (j = 0; j < items; j++) {
            assert(n != NULL); /* scan-build */
            if (n->name && (j != (items - 1))) /* 2nd comparison is to let separators through */
            {
                n2 = n;
                while (n2) {
                    if (!strcmp(n2->name, parts[j]))
                        break;
                    n2 = n2->next;
                }
            } else {
                n2 = NULL;
            }

            if (!n2) {
                n3 = (j != (items - 1)) ? calloc_2(1, sizeof(struct menu_item_t)) : NULL;

                assert(n != NULL); /* scan-build */
                if (n->name) {
                    while (n->next) {
                        n = n->next;
                    }
                    n->next = calloc_2(1, sizeof(struct menu_item_t));
                    n = n->next;
                }
                n->name = strdup_2(parts[j]);
                n->child = n3;

                n2 = n;
                n = n3;
            } else {
                n = n2->child;
            }
        }

        if (n2) /* scan-build */
        {
            n2->idx = i;
            n2->valid = 1;
        }

        free_decomposed_path(items, parts);
    }

    menubar = alt_menu_walk(mi, wlist, mtree, is_menubar ? 0 : 1, accel); /* returns a menubar */

    return (menubar);
}

GtkWidget *alt_menu_top(GtkWidget *window)
{
    gtkwave_mlist_t *mi = menu_items;
    int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
    GtkWidget *menubar;
    GtkAccelGroup *global_accel = gtk_accel_group_new();
    int i;
    GtkWidget *mw;

    menu_wlist = calloc(nmenu_items, sizeof(GtkWidget *)); /* calloc, not calloc_2() */

    menubar = alt_menu(mi, nmenu_items, menu_wlist, global_accel, TRUE);

    GLOBALS->regexp_string_menu_c_1 = calloc_2(1, 129);

    if (GLOBALS->loaded_file_type == MISSING_FILE) {
        for (i = 0; i < nmenu_items; i++) {
            switch (i) {
                case WV_MENU_FONVT:
                case WV_MENU_WCLOSE:
#if defined(HAVE_LIBTCL)
                case WV_MENU_TCLSCR:
#endif
                case WV_MENU_FQY:
#ifdef MAC_INTEGRATION
                case WV_MENU_HWM:
#endif
                case WV_MENU_HWV:
                    break;

                default:
                    mw = menu_wlist[i];
                    if (mw) {
#ifdef MAC_INTEGRATION
                        if (menu_items[i].callback)
#endif
                        {
                            gtk_widget_set_sensitive(mw, FALSE);
                        }
                    }
                    break;
            }
        }

#ifdef WAVE_USE_MENU_BLACKOUTS
        for (i = 0; i < (sizeof(menu_blackouts) / sizeof(char *)); i++) {
            mw = menu_wlist[i];
            if (mw) {
                gtk_widget_set_sensitive(mw, FALSE);
            }
        }
#endif
    }

    if ((GLOBALS->socket_xid) || (GLOBALS->partial_vcd)) {
        gtk_widget_destroy(menu_wlist[WV_MENU_FONVT]);
        menu_wlist[WV_MENU_FONVT] = NULL;
    }

    if (!GLOBALS->partial_vcd) {
        gtk_widget_destroy(menu_wlist[WV_MENU_VZDYN]);
        menu_wlist[WV_MENU_VZDYN] = NULL;
        gtk_widget_destroy(menu_wlist[WV_MENU_VZDYNE]);
        menu_wlist[WV_MENU_VZDYNE] = NULL;
    }

    if (GLOBALS->loaded_file_type == DUMPLESS_FILE) {
        gtk_widget_destroy(menu_wlist[WV_MENU_FRW]);
        menu_wlist[WV_MENU_FRW] = NULL;
    }

    gtk_window_add_accel_group(GTK_WINDOW(window), global_accel);

#ifdef MAC_INTEGRATION
#if defined(HAVE_LIBTCL)
    gtk_widget_hide(menu_wlist[WV_MENU_TCLSEP]);
#endif
    gtk_widget_hide(menu_wlist[WV_MENU_FQY]);
#endif

    set_menu_toggles();

    return (menubar);
}

#ifdef MAC_INTEGRATION
void osx_menu_sensitivity(gboolean tr)
{
    GtkWidget *mw;
    int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
    int i;

    for (i = 0; i < nmenu_items; i++) {
        mw = menu_wlist[i];
        if (mw) {
            if (menu_items[i].callback) {
                gtk_widget_set_sensitive(mw, tr);
            }
        }
    }
}
#endif

void wave_gtk_grab_add(GtkWidget *w)
{
    gtk_grab_add(w);

#ifdef MAC_INTEGRATION
    osx_menu_sensitivity(FALSE);
#endif
}

void wave_gtk_grab_remove(GtkWidget *w)
{
    gtk_grab_remove(w);

#ifdef MAC_INTEGRATION
    if (GLOBALS->loaded_file_type != MISSING_FILE) {
        osx_menu_sensitivity(TRUE);
    } else {
        int i;
        GtkWidget *mw;
        int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);

        for (i = 0; i < nmenu_items; i++) {
            switch (i) {
                case WV_MENU_FONVT:
                case WV_MENU_WCLOSE:
#if defined(HAVE_LIBTCL)
                case WV_MENU_TCLSCR:
#endif
                case WV_MENU_FQY:
#ifdef MAC_INTEGRATION
                case WV_MENU_HWM:
#endif
                case WV_MENU_HWV:
                    mw = menu_wlist[i];
                    if (mw) {
#ifdef MAC_INTEGRATION
                        if (menu_items[i].callback)
#endif
                        {
                            gtk_widget_set_sensitive(mw, TRUE);
                        }
                    }
                    break;

                default:
                    break;
            }
        }
    }
#endif
}

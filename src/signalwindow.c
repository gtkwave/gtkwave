/*
 * Copyright (c) Tony Bybell 1999-2018.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <gdk/gdkkeysyms.h>
#include "gtk23compat.h"
#include "currenttime.h"
#include "pixmaps.h"
#include "symbol.h"
#include "debug.h"
#include "signal_list.h"
#include "gw-wave-view.h"

/* GDK_KEY_equal defined from gtk2 2.22 onwards. */
#ifndef GDK_KEY_equal
#define GDK_KEY_equal GDK_equal
#endif
#ifndef GDK_KEY_Up
#define GDK_KEY_Up GDK_Up
#endif
#ifndef GDK_KEY_KP_Up
#define GDK_KEY_KP_Up GDK_KP_Up
#endif
#ifndef GDK_KEY_Down
#define GDK_KEY_Down GDK_Down
#endif
#ifndef GDK_KEY_KP_Down
#define GDK_KEY_KP_Down GDK_KP_Down
#endif

#undef FOCUS_DEBUG_MSGS

/*
 * complain about certain ops conflict with dnd...
 */
void dnd_error(void)
{
    status_text("Can't perform that operation when waveform drag and drop is in progress!\n");
}

/**************************************************************************/
/***  standard click routines turned on with "use_standard_clicking"=1  ***/

static gboolean mouseover_timer(gpointer dummy)
{
    (void)dummy;

    static gboolean run_once = FALSE;
    gdouble x, y;
    GdkModifierType state;
    GwTrace t_trans;

    gint xi, yi;

    if (GLOBALS->button2_debounce_flag) {
        GLOBALS->button2_debounce_flag = 0;
    }

    if (in_main_iteration())
        return (TRUE);

    if (GLOBALS->splash_is_loading) {
        return (TRUE);
    }

    if (GLOBALS->splash_fix_win_title) {
        GLOBALS->splash_fix_win_title = 0;
        wave_gtk_window_set_title(GTK_WINDOW(GLOBALS->mainwindow),
                                  GLOBALS->winname,
                                  GLOBALS->dumpfile_is_modified ? WAVE_SET_TITLE_MODIFIED
                                                                : WAVE_SET_TITLE_NONE,
                                  0);
    }

    if (process_finder_names_queued()) {
        if (GLOBALS->pFileChoose) {
            if (!GLOBALS->window_simplereq_c_9) {
                char *qn = process_finder_extract_queued_name();
                if (qn) {
                    int qn_len = strlen(qn);
                    const int mlen = 30;
                    if (qn_len < mlen) {
                        simplereqbox("File queued for loading", 300, qn, "OK", NULL, NULL, 1);
                    } else {
                        char *qn_2 = g_alloca(mlen + 4);
                        strcpy(qn_2, "...");
                        strcat(qn_2, qn + qn_len - mlen);
                        simplereqbox("File queued for loading", 300, qn_2, "OK", NULL, NULL, 1);
                    }
                    return (TRUE);
                }
            }
        } else {
            if (process_finder_name_integration()) {
                return (TRUE);
            }
        }
    }

    if (GLOBALS->loaded_file_type == MISSING_FILE) {
        return (TRUE);
    }

    if (run_once == FALSE) /* avoid any race conditions with the toolkit for uninitialized data */
    {
        run_once = TRUE;
        return (TRUE);
    }

    if ((!GLOBALS->signalarea) || (!gtk_widget_get_window(GLOBALS->signalarea))) {
        return (TRUE);
    }

    if (GLOBALS->mouseover_counter < 0)
        return (TRUE); /* mouseover is up in wave window so don't bother */

    WAVE_GDK_GET_POINTER(gtk_widget_get_window(GLOBALS->signalarea), &x, &y, &xi, &yi, &state);
    WAVE_GDK_GET_POINTER_COPY;

    GLOBALS->mouseover_counter = MIN(GLOBALS->mouseover_counter + 1, 10);

    GtkAllocation s_allocation;
    gtk_widget_get_allocation(GLOBALS->signalarea, &s_allocation);

    if (!((x >= 0) && (x < s_allocation.width) && (y >= 0) && (y < s_allocation.height))) {
        move_mouseover_sigs(NULL, 0, 0, GW_TIME_CONSTANT(0));
        GLOBALS->mouseover_counter = 0;
    } else if (GLOBALS->mouseover_counter >= 10) {
        GtkAllocation allocation;
        gtk_widget_get_allocation(GLOBALS->wavearea, &allocation);
        int num_traces_displayable = allocation.height / (GLOBALS->fontheight);
        int yr = GLOBALS->cached_mouseover_y;
        int i;
        GwTrace *t = NULL;

        num_traces_displayable--; /* for the time trace that is always there */

        yr -= GLOBALS->fontheight;
        if (yr < 0)
            goto bot;
        yr /= GLOBALS->fontheight; /* y now indicates the trace in question */
        if (yr > num_traces_displayable)
            goto bot;

        t = gw_signal_list_get_trace(GW_SIGNAL_LIST(GLOBALS->signalarea), 0);

        for (i = 0; i < yr; i++) {
            if (!t)
                goto bot;
            t = GiveNextTrace(t);
        }

        if (!t)
            goto bot;
        if ((t->flags & (/*TR_BLANK|*/ TR_EXCLUDE))) /* TR_BLANK removed because of transaction
                                                        handling below... */
        {
            t = NULL;
            goto bot;
        }

        if (t->flags &
            (TR_BLANK | TR_ANALOG_BLANK_STRETCH)) /* seek to real xact trace if present... */
        {
            GwTrace *tscan = t;
            int bcnt = 0;
            while ((tscan) && (tscan = GivePrevTrace(tscan))) {
                if (!(tscan->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH))) {
                    if (tscan->flags & TR_TTRANSLATED) {
                        break; /* found it */
                    } else {
                        tscan = NULL;
                    }
                } else {
                    bcnt++; /* bcnt is number of blank traces */
                }
            }

            if ((tscan) && (tscan->vector)) {
                GwBitVector *bv = tscan->n.vec;
                do {
                    bv = bv->transaction_chain; /* correlate to blank trace */
                } while (bv && (bcnt--));
                if (bv) {
                    memcpy(&t_trans,
                           tscan,
                           sizeof(GwTrace)); /* substitute into a synthetic trace */
                    t_trans.n.vec = bv;
                    t_trans.vector = 1;

                    t_trans.name = bv->bvname;
                    if (GLOBALS->hier_max_level)
                        t_trans.name = hier_extract(t_trans.name, GLOBALS->hier_max_level);

                    t = &t_trans;
                    goto bot; /* is goto process_trace; in wavewindow.c */
                }
            }
        }

        if ((t->flags & TR_BLANK)) {
            t = NULL;
            goto bot;
        }

        if (t->flags & TR_ANALOG_BLANK_STRETCH) /* seek to real analog trace is present... */
        {
            while ((t) && (t = t->t_prev)) {
                if (!(t->flags & TR_ANALOG_BLANK_STRETCH)) {
                    if (t->flags & TR_ANALOGMASK) {
                        break; /* found it */
                    } else {
                        t = NULL;
                    }
                }
            }
        }

    bot:
        if (t) {
            // TODO: don't use sentinel value for disabled primary marker
            GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
            GwTime primary_pos =
                gw_marker_is_enabled(primary_marker) ? gw_marker_get_position(primary_marker) : -1;

            move_mouseover_sigs(t,
                                GLOBALS->cached_mouseover_x,
                                GLOBALS->cached_mouseover_y,
                                primary_pos);
        } else {
            move_mouseover_sigs(NULL, 0, 0, GW_TIME_CONSTANT(0));
        }
    }

    return (TRUE);
}

/***  standard click routines turned on with "use_standard_clicking"=1  ***/
/**************************************************************************/

/**************************************************************************/
/***  standard click routines turned on with "use_standard_clicking"=0  ***/
/***                                                                    ***/
/***                        no longer supported                         ***/
/***                                                                    ***/
/***  gtkwave click routines turned on with "use_standard_clicking"=0   ***/
/**************************************************************************/

GtkWidget *create_signalwindow(void)
{
    GLOBALS->signalarea = gw_signal_list_new();

    MaxSignalLength();

    g_timeout_add(100, mouseover_timer, NULL);

    GtkAdjustment *hslider = gw_signal_list_get_hadjustment(GW_SIGNAL_LIST(GLOBALS->signalarea));
    GtkWidget *hscroll = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, hslider);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(box), GLOBALS->signalarea, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), hscroll, FALSE, FALSE, 0);

    gtk_widget_show_all(box);

    return box;
}

void redraw_signals_and_waves(void)
{
    if (!GLOBALS->signalarea || !GLOBALS->wavewindow) {
        return;
    }

    UpdateTracesVisible();

    MaxSignalLength();
    gw_signal_list_force_redraw(GW_SIGNAL_LIST(GLOBALS->signalarea));
    gw_wave_view_force_redraw(GW_WAVE_VIEW(GLOBALS->wavearea));
}
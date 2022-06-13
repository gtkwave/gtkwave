/*
 * Copyright (c) Tony Bybell 1999-2016.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include <string.h>
#include "gtk23compat.h"
#include "debug.h"
#include "analyzer.h"
#include "currenttime.h"

static void gtkwave_strrev(char *p)
{
    char *q = p;
    while (q && *q)
        ++q;
    for (--q; p < q; ++p, --q)
        *p = *p ^ *q, *q = *p ^ *q, *p = *p ^ *q;
}

char *make_bijective_marker_id_string(char *buf, unsigned int value)
{
    char *pnt = buf;

    value++; /* bijective values start at one */
    while (value) {
        value--;
        *(pnt++) = (char)('A' + value % ('Z' - 'A' + 1));
        value = value / ('Z' - 'A' + 1);
    }

    *pnt = 0;
    gtkwave_strrev(buf);
    return (buf);
}

unsigned int bijective_marker_id_string_hash(const char *so)
{
    unsigned int val = 0;
    int i;
    int len = strlen(so);
    char sn[16];
    char *s = sn;

    strcpy(sn, so);
    gtkwave_strrev(sn);

    s += len;
    for (i = 0; i < len; i++) {
        char c = toupper(*(--s));
        if ((c < 'A') || (c > 'Z'))
            break;
        val *= ('Z' - 'A' + 1);
        val += ((unsigned char)c) - ('A' - 1);
    }

    val--; /* bijective values start at one so decrement */
    return (val);
}

unsigned int bijective_marker_id_string_len(const char *s)
{
    int len = 0;

    while (*s) {
        char c = toupper(*s);
        if ((c >= 'A') && (c <= 'Z')) {
            len++;
            s++;
            continue;
        } else {
            break;
        }
    }

    return (len);
}

struct _GwMarkerDialog
{
    GtkDialog parent_instance;

    GtkWidget *time_entries[WAVE_NUM_NAMED_MARKERS];
    GtkWidget *name_entries[WAVE_NUM_NAMED_MARKERS];
};

G_DEFINE_FINAL_TYPE(GwMarkerDialog, gw_marker_dialog, GTK_TYPE_DIALOG)

static void time_entry_activate(GtkWidget *widget, GwMarkerDialog *marker_dialog)
{
    int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "index"));

    TimeType time = gw_marker_dialog_get_time(marker_dialog, index);

    if (time >= 0) {
        char buf[49];

        reformat_time_simple(buf, time + GLOBALS->global_time_offset, GLOBALS->time_dimension);

        gtk_entry_set_text(GTK_ENTRY(widget), buf);
    } else {
        gtk_entry_set_text(GTK_ENTRY(widget), "");
    }
}

static gboolean time_entry_focus_out_event(GtkWidget *widget,
                                           GdkEventFocus *event,
                                           GwMarkerDialog *marker_dialog)
{
    (void)event;

    time_entry_activate(widget, marker_dialog);

    return GDK_EVENT_PROPAGATE;
}

static void gw_marker_dialog_class_init(GwMarkerDialogClass *klass)
{
    (void)klass;
}

static void gw_marker_dialog_init(GwMarkerDialog *marker_dialog)
{
    gtk_window_set_title(GTK_WINDOW(marker_dialog), "Markers");
    gtk_window_set_destroy_with_parent(GTK_WINDOW(marker_dialog), TRUE);

    gtk_dialog_add_buttons(GTK_DIALOG(marker_dialog),
                           "Cancel",
                           GTK_RESPONSE_CANCEL,
                           "Ok",
                           GTK_RESPONSE_OK,
                           NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(marker_dialog), GTK_RESPONSE_OK);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);

    for (int i = 0; i < WAVE_NUM_NAMED_MARKERS; i++) {
        char label_text[16];
        make_bijective_marker_id_string(label_text, i);

        GtkWidget *label = gtk_label_new(label_text);
        gtk_grid_attach_next_to(GTK_GRID(grid), label, NULL, GTK_POS_BOTTOM, 1, 1);

        GtkWidget *time_entry = gtk_entry_new();
        g_object_set_data(G_OBJECT(time_entry), "index", GINT_TO_POINTER(i));
        gtk_entry_set_placeholder_text(GTK_ENTRY(time_entry), "<None>");
        gtk_widget_set_hexpand(GTK_WIDGET(time_entry), TRUE);
        gtk_grid_attach_next_to(GTK_GRID(grid), time_entry, label, GTK_POS_RIGHT, 1, 1);

        TimeType time = GLOBALS->named_markers[i];
        if (time >= 0) {
            char buf[49];

            reformat_time_simple(buf, time + GLOBALS->global_time_offset, GLOBALS->time_dimension);

            gtk_entry_set_text(GTK_ENTRY(time_entry), buf);
        }

        g_signal_connect(time_entry, "activate", G_CALLBACK(time_entry_activate), marker_dialog);
        g_signal_connect(time_entry,
                         "focus-out-event",
                         G_CALLBACK(time_entry_focus_out_event),
                         marker_dialog);

        GtkWidget *name_entry = gtk_entry_new();
        gtk_widget_set_hexpand(GTK_WIDGET(name_entry), TRUE);
        gtk_grid_attach_next_to(GTK_GRID(grid), name_entry, time_entry, GTK_POS_RIGHT, 1, 1);

        if (GLOBALS->marker_names[i] != NULL) {
            gtk_entry_set_text(GTK_ENTRY(name_entry), GLOBALS->marker_names[i]);
        }

        marker_dialog->name_entries[i] = name_entry;
        marker_dialog->time_entries[i] = time_entry;
    }

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled_window, 400, 300);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), grid);

    GtkWidget *widget = gtk_dialog_get_content_area(GTK_DIALOG(marker_dialog));
    gtk_container_add(GTK_CONTAINER(widget), scrolled_window);

    gtk_widget_show_all(scrolled_window);
}

GwMarkerDialog *gw_marker_dialog_new(void)
{
    return GW_MARKER_DIALOG(g_object_new(GW_TYPE_MARKER_DIALOG, "use-header-bar", TRUE, NULL));
}

TimeType gw_marker_dialog_get_time(GwMarkerDialog *marker_dialog, int index)
{
    g_return_val_if_fail(GW_IS_MARKER_DIALOG(marker_dialog), -1);
    g_return_val_if_fail(index >= 0 && index < WAVE_NUM_NAMED_MARKERS, -1);

    const gchar *text = gtk_entry_get_text(GTK_ENTRY(marker_dialog->time_entries[index]));

    if (g_utf8_strlen(text, -1) == 0 || !g_ascii_isdigit(text[0])) {
        return -1;
    }

    TimeType time = unformat_time(text, GLOBALS->time_dimension);
    time -= GLOBALS->global_time_offset;
    if (time < GLOBALS->tims.start || time > GLOBALS->tims.last) {
        return -1;
    }

    return time;
}

const gchar *gw_marker_dialog_get_name(GwMarkerDialog *marker_dialog, int index)
{
    g_return_val_if_fail(GW_IS_MARKER_DIALOG(marker_dialog), NULL);
    g_return_val_if_fail(index >= 0 && index < WAVE_NUM_NAMED_MARKERS, NULL);

    return gtk_entry_get_text(GTK_ENTRY(marker_dialog->name_entries[index]));
}

void gw_show_marker_dialog(void)
{
    GwMarkerDialog *marker_dialog = gw_marker_dialog_new();
    gtk_window_set_transient_for(GTK_WINDOW(marker_dialog), GTK_WINDOW(GLOBALS->mainwindow));

    if (gtk_dialog_run(GTK_DIALOG(marker_dialog)) == GTK_RESPONSE_OK) {
        for (int i = 0; i < WAVE_NUM_NAMED_MARKERS; i++) {
            GLOBALS->named_markers[i] = gw_marker_dialog_get_time(marker_dialog, i);

            if (GLOBALS->marker_names[i] != NULL) {
                free_2(GLOBALS->marker_names[i]);
                GLOBALS->marker_names[i] = NULL;
            }

            const gchar *name = gw_marker_dialog_get_name(marker_dialog, i);

            if (g_utf8_strlen(name, -1) > 0) {
                GLOBALS->marker_names[i] = strdup_2(name);
            }
        }

        redraw_signals_and_waves();
    }

    gtk_widget_destroy(GTK_WIDGET(marker_dialog));
}

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

static void str_change_callback(GtkWidget *entry, gpointer which)
{
    const gchar *entry_text;
    int i;
    uint32_t hashmask = WAVE_NUM_NAMED_MARKERS;
    hashmask |= hashmask >> 1;
    hashmask |= hashmask >> 2;
    hashmask |= hashmask >> 4;
    hashmask |= hashmask >> 8;
    hashmask |= hashmask >> 16;

    i = ((int)(((intptr_t)which) & hashmask)) % WAVE_NUM_NAMED_MARKERS;
    GLOBALS->dirty_markerbox_c_1 = 1;

    entry_text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (entry_text && strlen(entry_text)) {
        if (GLOBALS->shadow_marker_names[i]) {
            free_2(GLOBALS->shadow_marker_names[i]);
        }

        GLOBALS->shadow_marker_names[i] = strdup_2(entry_text);
    } else {
        if (GLOBALS->shadow_marker_names[i]) {
            free_2(GLOBALS->shadow_marker_names[i]);
            GLOBALS->shadow_marker_names[i] = NULL;
        }
    }
}

static void str_enter_callback(GtkWidget *entry, gpointer which)
{
    const gchar *entry_text;
    int i;
    uint32_t hashmask = WAVE_NUM_NAMED_MARKERS;
    hashmask |= hashmask >> 1;
    hashmask |= hashmask >> 2;
    hashmask |= hashmask >> 4;
    hashmask |= hashmask >> 8;
    hashmask |= hashmask >> 16;

    i = ((int)(((intptr_t)which) & hashmask)) % WAVE_NUM_NAMED_MARKERS;
    GLOBALS->dirty_markerbox_c_1 = 1;

    entry_text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (entry_text && strlen(entry_text)) {
        if (GLOBALS->shadow_marker_names[i]) {
            free_2(GLOBALS->shadow_marker_names[i]);
        }

        GLOBALS->shadow_marker_names[i] = strdup_2(entry_text);
        gtk_editable_select_region(GTK_EDITABLE(entry),
                                   0,
                                   gtk_entry_get_text_length(GTK_ENTRY(entry)));

    } else {
        if (GLOBALS->shadow_marker_names[i]) {
            free_2(GLOBALS->shadow_marker_names[i]);
            GLOBALS->shadow_marker_names[i] = NULL;
        }
    }
}

static void change_callback(GtkWidget *widget, gpointer which)
{
    (void)widget;

    GtkWidget *entry;
    TimeType temp;
    const gchar *entry_text;
    char buf[49];
    int i;
    int ent_idx;
    uint32_t hashmask = WAVE_NUM_NAMED_MARKERS;
    hashmask |= hashmask >> 1;
    hashmask |= hashmask >> 2;
    hashmask |= hashmask >> 4;
    hashmask |= hashmask >> 8;
    hashmask |= hashmask >> 16;

    ent_idx = ((int)(((intptr_t)which) & hashmask)) % WAVE_NUM_NAMED_MARKERS;

    entry = GLOBALS->entries_markerbox_c_1[ent_idx];

    entry_text = gtk_entry_get_text(GTK_ENTRY(entry));
    entry_text = entry_text ? entry_text : "";
    if (!strlen(entry_text))
        goto failure;
    if (entry_text[0] != '-') {
        if (!isdigit((int)(unsigned char)entry_text[0]))
            goto failure;
    }

    temp = unformat_time(entry_text, GLOBALS->time_dimension);
    temp -= GLOBALS->global_time_offset;
    if ((temp < GLOBALS->tims.start) || (temp > GLOBALS->tims.last))
        goto failure;

    for (i = 0; i < WAVE_NUM_NAMED_MARKERS; i++) {
        if (temp == GLOBALS->shadow_markers_markerbox_c_1[i]) {
            if (i != ent_idx) {
                GLOBALS->shadow_markers_markerbox_c_1[ent_idx] = -1;
            }
            goto failure;
        }
    }

    reformat_time_simple(buf, temp + GLOBALS->global_time_offset, GLOBALS->time_dimension);

    GLOBALS->shadow_markers_markerbox_c_1[ent_idx] = temp;
    GLOBALS->dirty_markerbox_c_1 = 1;

failure:
    return;
}

static void enter_callback(GtkWidget *widget, gpointer which)
{
    (void)widget;

    GtkWidget *entry;
    /* TimeType *modify; */ /* scan-build */
    TimeType temp;
    const gchar *entry_text;
    char buf[49];
    int i;
    int ent_idx;
    uint32_t hashmask = WAVE_NUM_NAMED_MARKERS;
    hashmask |= hashmask >> 1;
    hashmask |= hashmask >> 2;
    hashmask |= hashmask >> 4;
    hashmask |= hashmask >> 8;
    hashmask |= hashmask >> 16;

    ent_idx = ((int)(((intptr_t)which) & hashmask)) % WAVE_NUM_NAMED_MARKERS;

    entry = GLOBALS->entries_markerbox_c_1[ent_idx];

    entry_text = gtk_entry_get_text(GTK_ENTRY(entry));
    entry_text = entry_text ? entry_text : "";
    if (!strlen(entry_text))
        goto failure;

    temp = unformat_time(entry_text, GLOBALS->time_dimension);
    temp -= GLOBALS->global_time_offset;
    if ((temp < GLOBALS->tims.start) || (temp > GLOBALS->tims.last))
        goto failure;

    for (i = 0; i < WAVE_NUM_NAMED_MARKERS; i++) {
        if (temp == GLOBALS->shadow_markers_markerbox_c_1[i])
            goto failure;
    }

    reformat_time_simple(buf, temp + GLOBALS->global_time_offset, GLOBALS->time_dimension);
    gtk_entry_set_text(GTK_ENTRY(entry), buf);

    GLOBALS->shadow_markers_markerbox_c_1[ent_idx] = temp;
    GLOBALS->dirty_markerbox_c_1 = 1;
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, gtk_entry_get_text_length(GTK_ENTRY(entry)));
    return;

failure:
    /* modify=(TimeType *)which; */ /* scan-build */
    if (GLOBALS->shadow_markers_markerbox_c_1[ent_idx] == -1) {
        sprintf(buf, "<None>");
    } else {
        reformat_time_simple(buf,
                             GLOBALS->shadow_markers_markerbox_c_1[ent_idx] +
                                 GLOBALS->global_time_offset,
                             GLOBALS->time_dimension);
    }
    gtk_entry_set_text(GTK_ENTRY(entry), buf);
}

static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)widget;
    (void)nothing;

    if (GLOBALS->dirty_markerbox_c_1) {
        int i;
        for (i = 0; i < WAVE_NUM_NAMED_MARKERS; i++) {
            GLOBALS->named_markers[i] = GLOBALS->shadow_markers_markerbox_c_1[i];
            if (GLOBALS->marker_names[i])
                free_2(GLOBALS->marker_names[i]);
            GLOBALS->marker_names[i] = GLOBALS->shadow_marker_names[i];
            GLOBALS->shadow_marker_names[i] = NULL;
        }
        redraw_signals_and_waves();
    }

    wave_gtk_grab_remove(GLOBALS->window_markerbox_c_4);
    gtk_widget_destroy(GLOBALS->window_markerbox_c_4);
    GLOBALS->window_markerbox_c_4 = NULL;

    GLOBALS->cleanup_markerbox_c_4();
}

static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)widget;
    (void)nothing;

    int i;
    for (i = 0; i < WAVE_NUM_NAMED_MARKERS; i++) {
        if (GLOBALS->marker_names[i])
            free_2(GLOBALS->marker_names[i]);
        GLOBALS->marker_names[i] = GLOBALS->shadow_marker_names[i];
        GLOBALS->shadow_marker_names[i] = NULL;
    }

    wave_gtk_grab_remove(GLOBALS->window_markerbox_c_4);
    gtk_widget_destroy(GLOBALS->window_markerbox_c_4);
    GLOBALS->window_markerbox_c_4 = NULL;
}

void markerbox(char *title, GCallback func)
{
    char labtitle[16];
    int i;

    GLOBALS->cleanup_markerbox_c_4 = func;
    GLOBALS->dirty_markerbox_c_1 = 0;

    for (i = 0; i < WAVE_NUM_NAMED_MARKERS; i++) {
        GLOBALS->shadow_markers_markerbox_c_1[i] = GLOBALS->named_markers[i];
        GLOBALS->shadow_marker_names[i] = strdup_2(GLOBALS->marker_names[i]);
    }

    /* fix problem where ungrab doesn't occur if button pressed + simultaneous accelerator key
     * occurs */
    if (GLOBALS->in_button_press_wavewindow_c_1) {
        XXX_gdk_pointer_ungrab(GDK_CURRENT_TIME);
    }

    /* create a new modal window */
    GLOBALS->window_markerbox_c_4 =
        gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window_markerbox_c_4,
                     ((char *)&GLOBALS->window_markerbox_c_4) - ((char *)GLOBALS));

    gtk_window_set_title(GTK_WINDOW(GLOBALS->window_markerbox_c_4), title);
    gtkwave_signal_connect(GLOBALS->window_markerbox_c_4,
                           "delete_event",
                           (GCallback)destroy_callback,
                           NULL);
    gtk_container_set_border_width(GTK_CONTAINER(GLOBALS->window_markerbox_c_4), 12);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_add(GTK_CONTAINER(GLOBALS->window_markerbox_c_4), main_vbox);

    GtkWidget *scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_win), GTK_SHADOW_IN);
    gtk_widget_set_size_request(GTK_WIDGET(scrolled_win), 400, 300);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_win, TRUE, TRUE, 0);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_top(grid, 12);
    gtk_widget_set_margin_bottom(grid, 12);
    gtk_container_add(GTK_CONTAINER(scrolled_win), grid);

    for (i = 0; i < WAVE_NUM_NAMED_MARKERS; i++) {
        char buf[49];

        if (i != 0) {
            GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
            gtk_grid_attach_next_to(GTK_GRID(grid), separator, NULL, GTK_POS_BOTTOM, 3, 1);
        }

        make_bijective_marker_id_string(labtitle, i);
        GtkWidget *label = gtk_label_new(labtitle);
        gtk_widget_set_margin_start(label, 12);

        gtk_grid_attach_next_to(GTK_GRID(grid), label, NULL, GTK_POS_BOTTOM, 1, 1);

        GtkWidget *entry = X_gtk_entry_new_with_max_length(48);
        gtk_widget_set_hexpand(entry, TRUE);
        GLOBALS->entries_markerbox_c_1[i] = entry; 
        gtkwave_signal_connect(entry,
                               "activate",
                               G_CALLBACK(enter_callback),
                               (void *)((intptr_t)i));
        gtkwave_signal_connect(entry,
                               "changed",
                               G_CALLBACK(change_callback),
                               (void *)((intptr_t)i));
        if (GLOBALS->shadow_markers_markerbox_c_1[i] == -1) {
            sprintf(buf, "<None>");
        } else {
            reformat_time_simple(buf,
                                 GLOBALS->shadow_markers_markerbox_c_1[i] +
                                     GLOBALS->global_time_offset,
                                 GLOBALS->time_dimension);
        }

        gtk_entry_set_text(GTK_ENTRY(entry), buf);

        gtk_grid_attach_next_to(GTK_GRID(grid), entry, label, GTK_POS_RIGHT, 1, 1);

        /* string part */
        GtkWidget *entry2 = X_gtk_entry_new_with_max_length(48);
        gtk_widget_set_hexpand(entry2, TRUE);
        gtk_widget_set_margin_end(entry2, 12);
        if (GLOBALS->shadow_marker_names[i]) {
            gtk_entry_set_text(GTK_ENTRY(entry2), GLOBALS->shadow_marker_names[i]);
        }
        gtkwave_signal_connect(entry2,
                               "activate",
                               G_CALLBACK(str_enter_callback),
                               (void *)((intptr_t)i));
        gtkwave_signal_connect(entry2,
                               "changed",
                               G_CALLBACK(str_change_callback),
                               (void *)((intptr_t)i));

        gtk_grid_attach_next_to(GTK_GRID(grid), entry2, entry, GTK_POS_RIGHT, 1, 1);
    }

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start(GTK_BOX(main_vbox), button_box, FALSE, FALSE, 0);

    GtkWidget *button1 = gtk_button_new_with_label("OK");
    gtk_widget_set_size_request(button1, 100, -1);
    gtkwave_signal_connect(button1, "clicked", G_CALLBACK(ok_callback), NULL);
    gtk_widget_set_can_default(button1, TRUE);
    gtkwave_signal_connect_object(button1, "realize", (GCallback)gtk_widget_grab_default, button1);
    gtk_box_pack_start(GTK_BOX(button_box), button1, TRUE, TRUE, 0);

    GtkWidget *button2 = gtk_button_new_with_label("Cancel");
    gtk_widget_set_size_request(button2, 100, -1);
    gtkwave_signal_connect(button2, "clicked", G_CALLBACK(destroy_callback), NULL);
    gtk_widget_set_can_default(button2, TRUE);
    gtk_box_pack_start(GTK_BOX(button_box), button2, TRUE, TRUE, 0);

    gtk_widget_show_all(GLOBALS->window_markerbox_c_4);
    wave_gtk_grab_add(GLOBALS->window_markerbox_c_4);
}

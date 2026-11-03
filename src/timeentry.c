/*
 * Copyright (c) Tony Bybell 1999-2016
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "globals.h"
#include <gtk/gtk.h>
#include "gtk23compat.h"
#include "symbol.h"
#include "debug.h"

void time_update(void)
{
    DEBUG(printf("Timeentry Configure Event\n"));

    calczoom(GLOBALS->tims.zoom);
    fix_wavehadj();
    g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "changed");
    g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "value_changed");
}

void from_entry_callback(GtkWidget *widget, GtkWidget *entry)
{
    (void)widget;

    const gchar *entry_text;
    GwTime newlo;
    char fromstr[40];

    entry_text = gtk_entry_get_text(GTK_ENTRY(entry));
    entry_text = entry_text ? entry_text : "";
    DEBUG(printf("From Entry contents: %s\n", entry_text));

    GwTime global_time_offset = gw_dump_file_get_global_time_offset(GLOBALS->dump_file);
    GwTimeDimension time_dimension = gw_dump_file_get_time_dimension(GLOBALS->dump_file);
    GwTimeRange *time_range = gw_dump_file_get_time_range(GLOBALS->dump_file);

    newlo = unformat_time(entry_text, time_dimension) - global_time_offset;
    newlo = MAX(newlo, gw_time_range_get_start(time_range));

    if (newlo < (GLOBALS->tims.last)) {
        GLOBALS->tims.first = newlo;
        if (GLOBALS->tims.start < GLOBALS->tims.first)
            GLOBALS->tims.timecache = GLOBALS->tims.start = GLOBALS->tims.first;

        reformat_time(fromstr, GLOBALS->tims.first + global_time_offset, time_dimension);
        gtk_entry_set_text(GTK_ENTRY(entry), fromstr);
        time_update();
    } else {
        reformat_time(fromstr, GLOBALS->tims.first + global_time_offset, time_dimension);
        gtk_entry_set_text(GTK_ENTRY(entry), fromstr);
    }
}

void to_entry_callback(GtkWidget *widget, GtkWidget *entry)
{
    (void)widget;

    const gchar *entry_text;
    GwTime newhi;
    char tostr[40];

    entry_text = gtk_entry_get_text(GTK_ENTRY(entry));
    entry_text = entry_text ? entry_text : "";
    DEBUG(printf("To Entry contents: %s\n", entry_text));

    GwTime global_time_offset = gw_dump_file_get_global_time_offset(GLOBALS->dump_file);
    GwTimeDimension time_dimension = gw_dump_file_get_time_dimension(GLOBALS->dump_file);
    GwTimeRange *time_range = gw_dump_file_get_time_range(GLOBALS->dump_file);

    newhi = unformat_time(entry_text, time_dimension) - global_time_offset;

    /* null string makes max time */
    if (newhi > gw_time_range_get_end(time_range) || strlen(entry_text) == 0) {
        newhi = gw_time_range_get_end(time_range);
    }

    if (newhi > (GLOBALS->tims.first)) {
        GLOBALS->tims.last = newhi;
        reformat_time(tostr, GLOBALS->tims.last + global_time_offset, time_dimension);
        gtk_entry_set_text(GTK_ENTRY(entry), tostr);
        time_update();
    } else {
        reformat_time(tostr, GLOBALS->tims.last + global_time_offset, time_dimension);
        gtk_entry_set_text(GTK_ENTRY(entry), tostr);
    }
}

typedef struct
{
    GtkWidget *dialog;
    GtkWidget *from_entry;
    GtkWidget *to_entry;
} TimeDialog;

static TimeDialog build_time_dialog(void)
{
    GtkWidget *dialog =
        gtk_dialog_new_with_buttons("Time Settings",
                                    GTK_WINDOW(GLOBALS->mainwindow),
                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                    "Cancel",
                                    GTK_RESPONSE_CANCEL,
                                    "Ok",
                                    GTK_RESPONSE_OK,
                                    NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 12);

    GwTime global_time_offset = 0;
    GwTimeDimension time_dimension = GW_TIME_DIMENSION_BASE;
    GwTimeRange *time_range = NULL;

    if (GLOBALS->dump_file != NULL) {
        global_time_offset = gw_dump_file_get_global_time_offset(GLOBALS->dump_file);
        time_dimension = gw_dump_file_get_time_dimension(GLOBALS->dump_file);
        time_range = gw_dump_file_get_time_range(GLOBALS->dump_file);
    }


    GtkWidget *header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header), "<b>Time Range</b>");
    gtk_label_set_xalign(GTK_LABEL(header), 0.0);
    gtk_widget_set_margin_bottom(header, 6);

    GtkWidget *from_label = gtk_label_new("From:");
    gtk_label_set_xalign(GTK_LABEL(from_label), 0.0);
    GtkWidget *from_entry = X_gtk_entry_new_with_max_length(40);
    gtk_widget_set_hexpand(from_entry, TRUE);
    gtk_entry_set_activates_default(GTK_ENTRY(from_entry), TRUE);

    GtkWidget *to_label = gtk_label_new("To:");
    gtk_label_set_xalign(GTK_LABEL(to_label), 0.0);
    GtkWidget *to_entry = X_gtk_entry_new_with_max_length(40);
    gtk_widget_set_hexpand(to_entry, TRUE);
    gtk_entry_set_activates_default(GTK_ENTRY(to_entry), TRUE);

    GtkWidget *grid = gtk_grid_new();
    gtk_widget_set_margin_bottom(grid, 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);

    gtk_grid_attach(GTK_GRID(grid), header, 0, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), from_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), from_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), to_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), to_entry, 1, 2, 1, 1);

    gtk_widget_show_all(grid);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(content_area), grid);

    char fromstr[32];
    char tostr[32];
    if (time_range != NULL) {
        reformat_time(fromstr, GLOBALS->tims.first + global_time_offset, time_dimension);
        reformat_time(tostr, GLOBALS->tims.last + global_time_offset, time_dimension);
    } else {
        strcpy(fromstr, "-");
        strcpy(tostr, "-");
    }
    gtk_entry_set_text(GTK_ENTRY(from_entry), fromstr);
    gtk_entry_set_text(GTK_ENTRY(to_entry), tostr);

    TimeDialog result = {
        .dialog = dialog,
        .from_entry = from_entry,
        .to_entry = to_entry,
    };

    return result;
}

void show_time_dialog(void)
{
    TimeDialog dialog = build_time_dialog();

    if (gtk_dialog_run(GTK_DIALOG(dialog.dialog)) == GTK_RESPONSE_OK) {
        from_entry_callback(NULL, dialog.from_entry);
        to_entry_callback(NULL, dialog.to_entry);
    }

    gtk_widget_destroy(dialog.dialog);
}

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

void update_endcap_times_for_partial_vcd(void)
{
    char str[40];

    GwTime global_time_offset = gw_dump_file_get_global_time_offset(GLOBALS->dump_file);
    GwTimeDimension time_dimension = gw_dump_file_get_time_dimension(GLOBALS->dump_file);

    if (GLOBALS->from_entry) {
        reformat_time(str, GLOBALS->tims.first + global_time_offset, time_dimension);
        gtk_entry_set_text(GTK_ENTRY(GLOBALS->from_entry), str);
    }

    if (GLOBALS->to_entry) {
        reformat_time(str, GLOBALS->tims.last + global_time_offset, time_dimension);
        gtk_entry_set_text(GTK_ENTRY(GLOBALS->to_entry), str);
    }
}

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

/* Create an entry box */
GtkWidget *create_entry_box(void)
{
    GtkWidget *label, *label2;
    GtkWidget *box, *box2;
    GtkWidget *mainbox;

    char fromstr[32], tostr[32];

    GwTime global_time_offset = 0;
    GwTimeDimension time_dimension = GW_TIME_DIMENSION_BASE;
    GwTimeRange *time_range = NULL;

    if (GLOBALS->dump_file != NULL) {
        global_time_offset = gw_dump_file_get_global_time_offset(GLOBALS->dump_file);
        time_dimension = gw_dump_file_get_time_dimension(GLOBALS->dump_file);
        time_range = gw_dump_file_get_time_range(GLOBALS->dump_file);
    }

    label = gtk_label_new("From:");
    GLOBALS->from_entry = X_gtk_entry_new_with_max_length(40);

    if (time_range != NULL) {
        reformat_time(fromstr,
                      gw_time_range_get_start(time_range) + global_time_offset,
                      time_dimension);
    } else {
        strcpy(fromstr, "-");
    }

    gtk_entry_set_text(GTK_ENTRY(GLOBALS->from_entry), fromstr);
    g_signal_connect(GLOBALS->from_entry,
                     "activate",
                     G_CALLBACK(from_entry_callback),
                     GLOBALS->from_entry);
    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(box), GLOBALS->from_entry, TRUE, TRUE, 0);
    gtk_widget_set_size_request(GTK_WIDGET(GLOBALS->from_entry), 90, 22);
    gtk_tooltips_set_tip_2(GLOBALS->from_entry, "Scroll Lower Bound (use MX for named marker X)");
    gtk_widget_show(GLOBALS->from_entry);

    label2 = gtk_label_new("To:");
    GLOBALS->to_entry = X_gtk_entry_new_with_max_length(40);

    if (time_range != NULL) {
        reformat_time(tostr,
                      gw_time_range_get_end(time_range) + global_time_offset,
                      time_dimension);
    } else {
        strcpy(tostr, "-");
    }

    gtk_entry_set_text(GTK_ENTRY(GLOBALS->to_entry), tostr);
    g_signal_connect(GLOBALS->to_entry,
                     "activate",
                     G_CALLBACK(to_entry_callback),
                     GLOBALS->to_entry);
    box2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(box2), label2, TRUE, TRUE, 0);
    gtk_widget_show(label2);
    gtk_box_pack_start(GTK_BOX(box2), GLOBALS->to_entry, TRUE, TRUE, 0);
    gtk_widget_set_size_request(GTK_WIDGET(GLOBALS->to_entry), 90, 22);
    gtk_tooltips_set_tip_2(GLOBALS->to_entry, "Scroll Upper Bound (use MX for named marker X)");
    gtk_widget_show(GLOBALS->to_entry);

    mainbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    gtk_box_pack_start(GTK_BOX(mainbox), box, TRUE, FALSE, 1);
    gtk_widget_show(box);
    gtk_box_pack_start(GTK_BOX(mainbox), box2, TRUE, FALSE, 1);
    gtk_widget_show(box2);

    return (mainbox);
}

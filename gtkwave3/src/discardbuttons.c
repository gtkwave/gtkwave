/*
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include "currenttime.h"
#include "pixmaps.h"
#include "debug.h"

/* Create actual buttons */
GtkWidget *
create_discard_buttons (void)
{
GtkWidget *table;
GtkWidget *table2;
GtkWidget *frame;
GtkWidget *main_vbox;
GtkWidget *b1;
GtkWidget *b2;
GtkWidget *pixmapwid1, *pixmapwid2;

GtkTooltips *tooltips;

tooltips=gtk_tooltips_new_2();
gtk_tooltips_set_delay_2(tooltips,1500);

pixmapwid1=gtk_pixmap_new(GLOBALS->larrow_pixmap, GLOBALS->larrow_mask);
gtk_widget_show(pixmapwid1);
pixmapwid2=gtk_pixmap_new(GLOBALS->rarrow_pixmap, GLOBALS->rarrow_mask);
gtk_widget_show(pixmapwid2);

/* Create a table to hold the text widget and scrollbars */
table = gtk_table_new (1, 1, FALSE);

main_vbox = gtk_vbox_new (FALSE, 1);
gtk_container_border_width (GTK_CONTAINER (main_vbox), 1);
gtk_container_add (GTK_CONTAINER (table), main_vbox);

frame = gtk_frame_new ("Disc ");
gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

gtk_widget_show (frame);
gtk_widget_show (main_vbox);

table2 = gtk_table_new (2, 1, FALSE);

b1 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b1), pixmapwid1);
gtk_table_attach (GTK_TABLE (table2), b1, 0, 1, 0, 1,
			GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b1), "clicked", GTK_SIGNAL_FUNC(discard_left), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b1, "Increase 'From' Time", NULL);
gtk_widget_show(b1);

b2 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b2), pixmapwid2);
gtk_table_attach (GTK_TABLE (table2), b2, 0, 1, 1, 2,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b2), "clicked", GTK_SIGNAL_FUNC(discard_right), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b2, "Decrease 'To' Time", NULL);
gtk_widget_show(b2);

gtk_container_add (GTK_CONTAINER (frame), table2);
gtk_widget_show(table2);
return(table);
}


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

pixmapwid1=gtk_image_new_from_pixbuf(GLOBALS->larrow_pixbuf);
gtk_widget_show(pixmapwid1);
pixmapwid2=gtk_image_new_from_pixbuf(GLOBALS->rarrow_pixbuf);
gtk_widget_show(pixmapwid2);

/* Create a table to hold the text widget and scrollbars */
table = XXX_gtk_table_new (1, 1, FALSE);

main_vbox = XXX_gtk_vbox_new (FALSE, 1);
gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 1);
gtk_container_add (GTK_CONTAINER (table), main_vbox);

frame = gtk_frame_new ("Disc ");
gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

gtk_widget_show (frame);
gtk_widget_show (main_vbox);

table2 = XXX_gtk_table_new (2, 1, FALSE);

b1 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b1), pixmapwid1);
XXX_gtk_table_attach (XXX_GTK_TABLE (table2), b1, 0, 1, 0, 1,
			GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
g_signal_connect_swapped (XXX_GTK_OBJECT (b1), "clicked", G_CALLBACK(discard_left), XXX_GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(b1, "Increase 'From' Time");
gtk_widget_show(b1);

b2 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b2), pixmapwid2);
XXX_gtk_table_attach (XXX_GTK_TABLE (table2), b2, 0, 1, 1, 2,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
g_signal_connect_swapped (XXX_GTK_OBJECT (b2), "clicked", G_CALLBACK(discard_right), XXX_GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(b2, "Decrease 'To' Time");
gtk_widget_show(b2);

gtk_container_add (GTK_CONTAINER (frame), table2);
gtk_widget_show(table2);
return(table);
}


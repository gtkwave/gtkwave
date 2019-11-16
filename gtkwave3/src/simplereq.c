/*
 * Copyright (c) Tony Bybell 1999-2012.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include "gtk12compat.h"
#include <gtk/gtk.h>
#include "menu.h"
#include "debug.h"
#include "pixmaps.h"

#ifdef MAC_INTEGRATION

#include <cocoa_misc.h>

#else

static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

  DEBUG(printf("OK\n"));
  wave_gtk_grab_remove(GLOBALS->window_simplereq_c_9);
  gtk_widget_destroy(GLOBALS->window_simplereq_c_9);
  GLOBALS->window_simplereq_c_9 = NULL;
  if(GLOBALS->cleanup)GLOBALS->cleanup(NULL,(gpointer)1);
}

static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

  DEBUG(printf("Cancel\n"));
  wave_gtk_grab_remove(GLOBALS->window_simplereq_c_9);
  gtk_widget_destroy(GLOBALS->window_simplereq_c_9);
  GLOBALS->window_simplereq_c_9 = NULL;
  if(GLOBALS->cleanup)GLOBALS->cleanup(NULL,NULL);
}
#endif

void simplereqbox(char *title, int width, char *default_text,
	char *oktext, char *canceltext, GtkSignalFunc func, int is_alert)
{
#ifndef MAC_INTEGRATION
    GtkWidget *vbox, *hbox;
    GtkWidget *button1, *button2;
    GtkWidget *label, *separator;
    GtkWidget *pixmapwid1;
#else
(void)width;
#endif

    if(GLOBALS->window_simplereq_c_9) return; /* only should happen with GtkPlug */

    GLOBALS->cleanup=WAVE_GTK_SFUNCAST(func);

    /* fix problem where ungrab doesn't occur if button pressed + simultaneous accelerator key occurs */
    if(GLOBALS->in_button_press_wavewindow_c_1) { gdk_pointer_ungrab(GDK_CURRENT_TIME); }

    if(GLOBALS->wave_script_args)
	{
	if(GLOBALS->cleanup)GLOBALS->cleanup(NULL,(gpointer)1);
	return;
	}

#ifdef MAC_INTEGRATION
    /* requester is modal so it will block */
    switch(gtk_simplereqbox_req_bridge(title, default_text, oktext, canceltext, is_alert))
	{
	case 1:	if(GLOBALS->cleanup)GLOBALS->cleanup(NULL,(gpointer)1);
		break;

	case 2:	if(GLOBALS->cleanup)GLOBALS->cleanup(NULL,NULL);
		break;

	default:
		break;
	}
#else

    /* create a new modal window */
    GLOBALS->window_simplereq_c_9 = gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window_simplereq_c_9, ((char *)&GLOBALS->window_simplereq_c_9) - ((char *)GLOBALS));

    gtk_window_set_transient_for(GTK_WINDOW(GLOBALS->window_simplereq_c_9), GTK_WINDOW(GLOBALS->mainwindow));
    gtk_widget_set_usize( GTK_WIDGET (GLOBALS->window_simplereq_c_9), width, 200 - 64); /* 200 is for 128 px icon */
    gtk_window_set_title(GTK_WINDOW (GLOBALS->window_simplereq_c_9), title);
    gtkwave_signal_connect(GTK_OBJECT (GLOBALS->window_simplereq_c_9), "delete_event",(GtkSignalFunc) destroy_callback, NULL);
    gtk_window_set_policy(GTK_WINDOW(GLOBALS->window_simplereq_c_9), FALSE, FALSE, FALSE);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (GLOBALS->window_simplereq_c_9), vbox);
    gtk_widget_show (vbox);

    label=gtk_label_new(default_text);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    if(is_alert)
	{
	pixmapwid1=gtk_pixmap_new(GLOBALS->wave_alert_pixmap, GLOBALS->wave_alert_mask);
	}
	else
	{
	pixmapwid1=gtk_pixmap_new(GLOBALS->wave_info_pixmap, GLOBALS->wave_info_mask);
	}
    gtk_widget_show(pixmapwid1);
    gtk_container_add (GTK_CONTAINER (vbox), pixmapwid1);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 0);
    gtk_widget_show (separator);

    hbox = gtk_hbox_new (FALSE, 1);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label (oktext);
    gtk_widget_set_usize(button1, 100, -1);
    gtkwave_signal_connect(GTK_OBJECT (button1), "clicked", GTK_SIGNAL_FUNC(ok_callback), NULL);
    gtk_widget_show (button1);
    gtk_container_add (GTK_CONTAINER (hbox), button1);
    GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);
    gtkwave_signal_connect_object (GTK_OBJECT (button1), "realize", (GtkSignalFunc) gtk_widget_grab_default, GTK_OBJECT (button1));

    if(canceltext)
	{
    	button2 = gtk_button_new_with_label (canceltext);
    	gtk_widget_set_usize(button2, 100, -1);
    	gtkwave_signal_connect(GTK_OBJECT (button2), "clicked", GTK_SIGNAL_FUNC(destroy_callback), NULL);
    	GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);
    	gtk_widget_show (button2);
    	gtk_container_add (GTK_CONTAINER (hbox), button2);
	}

    gtk_widget_show(GLOBALS->window_simplereq_c_9);
    wave_gtk_grab_add(GLOBALS->window_simplereq_c_9);
#endif
}


/*
 * Copyright (c) Tony Bybell 1999-2013.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include "gtk12compat.h"
#include "menu.h"
#include "debug.h"
#include <cocoa_misc.h>
#include <string.h>

#ifdef MAC_INTEGRATION
/* disabled for now as we can't get it to auto enable when it comes up */
#define WAVE_MAC_USE_ENTRY
#endif

#ifndef WAVE_MAC_USE_ENTRY
static gint keypress_local(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
(void)widget;
(void)event;
(void)data;

if(GLOBALS->window_entry_c_1)
	{
	gdk_window_raise(GLOBALS->window_entry_c_1->window);
	}

return(FALSE);
}
#endif

#ifndef WAVE_MAC_USE_ENTRY
static void enter_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

  G_CONST_RETURN gchar *entry_text;
  int len;
  entry_text = gtk_entry_get_text(GTK_ENTRY(GLOBALS->entry_entry_c_1));
  entry_text = entry_text ? entry_text : "";
  DEBUG(printf("Entry contents: %s\n", entry_text));
  if(!(len=strlen(entry_text))) GLOBALS->entrybox_text=NULL;
	else strcpy((GLOBALS->entrybox_text=(char *)malloc_2(len+1)),entry_text);

  wave_gtk_grab_remove(GLOBALS->window_entry_c_1);
  gtk_widget_destroy(GLOBALS->window_entry_c_1);
  GLOBALS->window_entry_c_1 = NULL;

  GLOBALS->cleanup_entry_c_1();
}

static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

  DEBUG(printf("Entry Cancel\n"));
  GLOBALS->entrybox_text=NULL;
  wave_gtk_grab_remove(GLOBALS->window_entry_c_1);
  gtk_widget_destroy(GLOBALS->window_entry_c_1);
  GLOBALS->window_entry_c_1 = NULL;
}
#endif

void entrybox(char *title, int width, char *dflt_text, char *comment, int maxch, GtkSignalFunc func)
{
#ifndef WAVE_MAC_USE_ENTRY
    GtkWidget *vbox, *hbox;
    GtkWidget *button1, *button2;
/*  int height = (comment) ? 75 : 60; */
    int height = -1; /* changed as recent gnome themes have taller window title bars so 75 is not enough */
#endif

    GLOBALS->cleanup_entry_c_1=func;

    /* fix problem where ungrab doesn't occur if button pressed + simultaneous accelerator key occurs */
    if(GLOBALS->in_button_press_wavewindow_c_1) { gdk_pointer_ungrab(GDK_CURRENT_TIME); }

    if(GLOBALS->wave_script_args)
	{
        char *s = NULL;

        while((!s)&&(GLOBALS->wave_script_args->curr)) s = wave_script_args_fgetmalloc_stripspaces(GLOBALS->wave_script_args);
	if(s)
		{
		fprintf(stderr, "GTKWAVE | Entry '%s'\n", s);
		GLOBALS->entrybox_text = s;
		GLOBALS->cleanup_entry_c_1();
		}
		else
		{
		GLOBALS->entrybox_text = NULL;
		}

	return;
	}

#ifdef WAVE_MAC_USE_ENTRY
{
char *out_text_entry = NULL;
entrybox_req_bridge(title, width, dflt_text, comment, maxch, &out_text_entry);
if(out_text_entry)
	{
	int len=strlen(out_text_entry);
	if(!len) GLOBALS->entrybox_text=NULL;
	else strcpy((GLOBALS->entrybox_text=(char *)malloc_2(len+1)),out_text_entry);
	free(out_text_entry);
	GLOBALS->cleanup_entry_c_1();
	}
	else
	{
	GLOBALS->entrybox_text = NULL;
	}
return;
}
#else

    /* create a new modal window */
    GLOBALS->window_entry_c_1 = gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window_entry_c_1, ((char *)&GLOBALS->window_entry_c_1) - ((char *)GLOBALS));

    gtk_widget_set_usize( GTK_WIDGET (GLOBALS->window_entry_c_1), width, height);
    gtk_window_set_title(GTK_WINDOW (GLOBALS->window_entry_c_1), title);
    gtkwave_signal_connect(GTK_OBJECT (GLOBALS->window_entry_c_1), "delete_event",(GtkSignalFunc) destroy_callback, NULL);
    gtk_window_set_policy(GTK_WINDOW(GLOBALS->window_entry_c_1), FALSE, FALSE, FALSE);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (GLOBALS->window_entry_c_1), vbox);
    gtk_widget_show (vbox);

    if (comment)
      {
	GtkWidget *label, *cbox;

	cbox = gtk_hbox_new (FALSE, 1);
	gtk_box_pack_start (GTK_BOX (vbox), cbox, FALSE, FALSE, 0);
	gtk_widget_show (cbox);

	label = gtk_label_new(comment);
	gtk_widget_show (label);

	gtk_container_add (GTK_CONTAINER (cbox), label);
	GTK_WIDGET_SET_FLAGS (label, GTK_CAN_DEFAULT);
      }

    GLOBALS->entry_entry_c_1 = gtk_entry_new_with_max_length (maxch);
    gtkwave_signal_connect(GTK_OBJECT(GLOBALS->entry_entry_c_1), "activate",GTK_SIGNAL_FUNC(enter_callback),GLOBALS->entry_entry_c_1);
    gtk_entry_set_text (GTK_ENTRY (GLOBALS->entry_entry_c_1), dflt_text);
    gtk_entry_select_region (GTK_ENTRY (GLOBALS->entry_entry_c_1),0, GTK_ENTRY(GLOBALS->entry_entry_c_1)->text_length);
    gtk_box_pack_start (GTK_BOX (vbox), GLOBALS->entry_entry_c_1, FALSE, FALSE, 0);
    gtk_widget_show (GLOBALS->entry_entry_c_1);

    hbox = gtk_hbox_new (FALSE, 1);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label ("OK");
    gtk_widget_set_usize(button1, 100, -1);
    gtkwave_signal_connect(GTK_OBJECT (button1), "clicked", GTK_SIGNAL_FUNC(enter_callback), NULL);
    gtk_widget_show (button1);
    gtk_container_add (GTK_CONTAINER (hbox), button1);
    GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);
    gtkwave_signal_connect_object (GTK_OBJECT (button1), "realize", (GtkSignalFunc) gtk_widget_grab_default, GTK_OBJECT (button1));


    button2 = gtk_button_new_with_label ("Cancel");
    gtk_widget_set_usize(button2, 100, -1);
    gtkwave_signal_connect(GTK_OBJECT (button2), "clicked", GTK_SIGNAL_FUNC(destroy_callback), NULL);
    GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);
    gtk_widget_show (button2);
    gtk_container_add (GTK_CONTAINER (hbox), button2);

    gtk_widget_show(GLOBALS->window_entry_c_1);
    wave_gtk_grab_add(GLOBALS->window_entry_c_1);
    gdk_window_raise(GLOBALS->window_entry_c_1->window);

    gtk_signal_connect(GTK_OBJECT(GLOBALS->window_entry_c_1), "key_press_event",GTK_SIGNAL_FUNC(keypress_local), NULL);

#endif
}


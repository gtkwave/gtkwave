/*
 * Copyright (c) Tony Bybell 1999-2008
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "symbol.h"
#include "currenttime.h"


/* Add some text to our text widget - this is a callback that is invoked
when our window is realized. We could also force our window to be
realized with gtk_widget_realize, but it would have to be part of
a hierarchy first */


void help_text(char *str)
{
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN)
gtk_text_buffer_insert (GTK_TEXT_VIEW (GLOBALS->text_help_c_1)->buffer, &GLOBALS->iter_help_c_1, str, -1);

GtkTextMark *mark = gtk_text_buffer_get_mark (gtk_text_view_get_buffer(GTK_TEXT_VIEW (GLOBALS->text_help_c_1)), "end");
gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (GLOBALS->text_help_c_1), GTK_TEXT_MARK(mark));
#else
gtk_text_insert (GTK_TEXT (GLOBALS->text_help_c_1), NULL, &GLOBALS->text_help_c_1->style->black, NULL, str, -1);
#endif

gdk_window_raise(GLOBALS->window_help_c_2->window);
}

void help_text_bold(char *str)
{
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN)
gtk_text_buffer_insert_with_tags (GTK_TEXT_VIEW (GLOBALS->text_help_c_1)->buffer, &GLOBALS->iter_help_c_1,
                                 str, -1, GLOBALS->bold_tag_help_c_1, NULL);

GtkTextMark *mark = gtk_text_buffer_get_mark (gtk_text_view_get_buffer(GTK_TEXT_VIEW (GLOBALS->text_help_c_1)), "end");
gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (GLOBALS->text_help_c_1), GTK_TEXT_MARK(mark));
#else
gtk_text_insert (GTK_TEXT (GLOBALS->text_help_c_1), NULL, &GLOBALS->text_help_c_1->style->fg[GTK_STATE_SELECTED], &GLOBALS->text_help_c_1->style->bg[GTK_STATE_SELECTED], str, -1);
#endif

gdk_window_raise(GLOBALS->window_help_c_2->window);
}

static void
help_realize_text (GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

if(GLOBALS->loaded_file_type == MISSING_FILE)
	{
	help_text("To load a dumpfile into the viewer, either drag the icon"
		  " for it from the desktop or use the appropriate option(s)"
		  " from the ");

	help_text_bold("File");

	help_text(" menu.\n\n");
	}

help_text("Click on any menu item or button that corresponds to a menu item"
		" for its full description.  Pressing a hotkey for a menu item"
		" is also allowed.");
}

/* Create a scrolled text area that displays a "message" */
static GtkWidget *create_help_text (void)
{
GtkWidget *table;

/* Create a table to hold the text widget and scrollbars */
table = gtk_table_new (1, 16, FALSE);

/* Put a text widget in the upper left hand corner. Note the use of
* GTK_SHRINK in the y direction */
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN)
GLOBALS->text_help_c_1 = gtk_text_view_new ();
gtk_text_view_set_editable (GTK_TEXT_VIEW(GLOBALS->text_help_c_1), FALSE);
gtk_text_buffer_get_start_iter (gtk_text_view_get_buffer(GTK_TEXT_VIEW (GLOBALS->text_help_c_1)), &GLOBALS->iter_help_c_1);
GLOBALS->bold_tag_help_c_1 = gtk_text_buffer_create_tag (GTK_TEXT_VIEW (GLOBALS->text_help_c_1)->buffer, "bold",
                                      "weight", PANGO_WEIGHT_BOLD, NULL);

GtkTextIter iter;
gtk_text_buffer_get_end_iter (gtk_text_view_get_buffer(GTK_TEXT_VIEW (GLOBALS->text_help_c_1)), &iter);
gtk_text_buffer_create_mark (gtk_text_view_get_buffer(GTK_TEXT_VIEW (GLOBALS->text_help_c_1)), "end", &iter, FALSE);
#else
GLOBALS->text_help_c_1 = gtk_text_new (NULL, NULL);
gtk_text_set_editable(GTK_TEXT(GLOBALS->text_help_c_1), FALSE);
#endif
gtk_table_attach (GTK_TABLE (table), GLOBALS->text_help_c_1, 0, 14, 0, 1,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_SHRINK | GTK_EXPAND, 0, 0);
gtk_widget_set_usize(GTK_WIDGET(GLOBALS->text_help_c_1), 100, 50);
gtk_widget_show (GLOBALS->text_help_c_1);

/* And a VScrollbar in the upper right */
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN)
{
GtkTextViewClass *tc = (GtkTextViewClass*)GTK_OBJECT_GET_CLASS(GTK_OBJECT(GLOBALS->text_help_c_1));

tc->set_scroll_adjustments(GTK_TEXT_VIEW (GLOBALS->text_help_c_1), NULL, NULL);
GLOBALS->vscrollbar_help_c_1 = gtk_vscrollbar_new (GTK_TEXT_VIEW (GLOBALS->text_help_c_1)->vadjustment);
}
#else
GLOBALS->vscrollbar_help_c_1 = gtk_vscrollbar_new (GTK_TEXT (GLOBALS->text_help_c_1)->vadj);
#endif
gtk_table_attach (GTK_TABLE (table), GLOBALS->vscrollbar_help_c_1, 15, 16, 0, 1,GTK_FILL, GTK_FILL | GTK_SHRINK | GTK_EXPAND, 0, 0);
gtk_widget_show (GLOBALS->vscrollbar_help_c_1);

/* Add a handler to put a message in the text widget when it is realized */
gtkwave_signal_connect (GTK_OBJECT (GLOBALS->text_help_c_1), "realize", GTK_SIGNAL_FUNC (help_realize_text), NULL);


#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN)
gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(GLOBALS->text_help_c_1), GTK_WRAP_WORD);
#else
gtk_text_set_word_wrap(GTK_TEXT(GLOBALS->text_help_c_1), TRUE);
#endif
return(table);
}

/***********************************************************************************/


static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

  GLOBALS->helpbox_is_active=0;
  DEBUG(printf("OK\n"));
  gtk_widget_destroy(GLOBALS->window_help_c_2);
  GLOBALS->window_help_c_2 = NULL;
}

void helpbox(char *title, int width, char *default_text)
{
    GtkWidget *vbox, *hbox;
    GtkWidget *button1;
    GtkWidget *label, *separator;
    GtkWidget *ctext;

    /* fix problem where ungrab doesn't occur if button pressed + simultaneous accelerator key occurs */
    if(GLOBALS->in_button_press_wavewindow_c_1) { gdk_pointer_ungrab(GDK_CURRENT_TIME); }

    if(GLOBALS->helpbox_is_active) return;
    GLOBALS->helpbox_is_active=1;

    /* create a new nonmodal window */
    GLOBALS->window_help_c_2 = gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window_help_c_2, ((char *)&GLOBALS->window_help_c_2) - ((char *)GLOBALS));

    gtk_widget_set_usize( GTK_WIDGET (GLOBALS->window_help_c_2), width, 400);
    gtk_window_set_title(GTK_WINDOW (GLOBALS->window_help_c_2), title);
    gtkwave_signal_connect(GTK_OBJECT (GLOBALS->window_help_c_2), "delete_event",(GtkSignalFunc) ok_callback, NULL);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (GLOBALS->window_help_c_2), vbox);
    gtk_widget_show (vbox);

    label=gtk_label_new(default_text);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 0);
    gtk_widget_show (separator);

    ctext=create_help_text();
    gtk_box_pack_start (GTK_BOX (vbox), ctext, TRUE, TRUE, 0);
    gtk_widget_show (ctext);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 0);
    gtk_widget_show (separator);

    hbox = gtk_hbox_new (FALSE, 1);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label ("Close Help");
    gtk_widget_set_usize(button1, 100, -1);
    gtkwave_signal_connect(GTK_OBJECT (button1), "clicked", GTK_SIGNAL_FUNC(ok_callback), NULL);
    gtk_widget_show (button1);
    gtk_container_add (GTK_CONTAINER (hbox), button1);
    GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);
    gtkwave_signal_connect_object (GTK_OBJECT (button1), "realize", (GtkSignalFunc) gtk_widget_grab_default, GTK_OBJECT (button1));

    gtk_widget_show(GLOBALS->window_help_c_2);
}


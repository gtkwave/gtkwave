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
#include "menu.h"
#include "debug.h"
#include <string.h>

void entrybox(char *title, int width, char *dflt_text, char *comment, int maxch, GCallback func)
{
    GtkWidget *dialog;
    GtkDialogFlags flags;
    GtkWidget *content;
    GtkWidget *entry;

    if (GLOBALS->entrybox_text != NULL) {
        free_2 (GLOBALS->entrybox_text);
        GLOBALS->entrybox_text = NULL;
    }

    if (GLOBALS->wave_script_args) {
        char *s = NULL;

        while (s == NULL && GLOBALS->wave_script_args->curr != NULL)
            s = wave_script_args_fgetmalloc_stripspaces (GLOBALS->wave_script_args);

    	if(s) {
            fprintf (stderr, "GTKWAVE | Entry '%s'\n", s);
            GLOBALS->entrybox_text = s;
            func ();
		} else {
		    GLOBALS->entrybox_text = NULL;
		}

	    return;
	}

#if GTK_CHECK_VERSION(3,12,0)
    flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_USE_HEADER_BAR;
#else
    flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
#endif

    dialog = gtk_dialog_new_with_buttons(
        title,
        GTK_WINDOW(GLOBALS->mainwindow),
        flags,
        "Cancel",
        GTK_RESPONSE_CANCEL,
        "OK",
        GTK_RESPONSE_OK,
        NULL
    );
    gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_container_set_border_width (GTK_CONTAINER(dialog), 12);

    content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    gtk_box_set_spacing (GTK_BOX (content), 6);

    if (comment != NULL) {
        GtkWidget *label = gtk_label_new (comment);
#if GTK_CHECK_VERSION(3,16,0)
        gtk_label_set_xalign (GTK_LABEL(label), 0.0);
#endif
        gtk_box_pack_start (GTK_BOX(content), label, FALSE, FALSE, 0);
    }

    entry = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (entry), dflt_text);
    gtk_entry_set_max_length (GTK_ENTRY(entry), maxch);
    gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
    gtk_widget_set_size_request (entry, width, -1);
    gtk_box_pack_start (GTK_BOX(content), entry, FALSE, FALSE, 0);

    gtk_widget_show_all (content);

    int result = gtk_dialog_run (GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_OK) {
        const gchar *text = gtk_entry_get_text (GTK_ENTRY (entry));
        int len = strlen (text);

        if (len > 0) {
            GLOBALS->entrybox_text = malloc_2 (len + 1);
            strcpy (GLOBALS->entrybox_text, text);
        }

        func ();
    }

    gtk_widget_destroy (dialog);
}


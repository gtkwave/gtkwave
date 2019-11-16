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
#include "symbol.h"
#include "lxt2_read.h"
#include "lx2.h"

/* Add some text to our text widget - this is a callback that is invoked
when our window is realized. We could also force our window to be
realized with gtk_widget_realize, but it would have to be part of
a hierarchy first */


void status_text(char *str)
{
if(!GLOBALS->quiet_checkmenu) /* when gtkwave_mlist_t check menuitems are being initialized */
	{
	int len = strlen(str);
	char ch = len ? str[len-1] : 0;

	if(GLOBALS->text_status_c_2)
		{
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN)
	        gtk_text_buffer_insert (GTK_TEXT_VIEW (GLOBALS->text_status_c_2)->buffer, &GLOBALS->iter_status_c_3, str, -1);

		GtkTextMark *mark = gtk_text_buffer_get_mark (gtk_text_view_get_buffer(GTK_TEXT_VIEW (GLOBALS->text_status_c_2)), "end");
		gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (GLOBALS->text_status_c_2), GTK_TEXT_MARK(mark));
#else
	        gtk_text_insert (GTK_TEXT (GLOBALS->text_status_c_2), NULL, &GLOBALS->text_status_c_2->style->black, NULL, str, -1);
#endif
		}
		else
		{
		fprintf(stderr, "GTKWAVE | %s%s", str, (ch=='\n') ? "" : "\n");
		}

	{
	char *stemp = wave_alloca(len+1);
	strcpy(stemp, str);

	if(ch == '\n')
		{
		stemp[len-1] = 0;
		}
	gtkwavetcl_setvar(WAVE_TCLCB_STATUS_TEXT, stemp, WAVE_TCLCB_STATUS_TEXT_FLAGS);
	}
	}
}

void
realize_text (GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

char buf[128];

if(GLOBALS->is_vcd)
	{
	if(GLOBALS->partial_vcd)
		{
		status_text("VCD loading interactively.\n");
		}
		else
		{
		status_text("VCD loaded successfully.\n");
		}
	}
else
if(GLOBALS->is_lxt)
	{
	status_text("LXT loaded successfully.\n");
	}
else
if(GLOBALS->is_ghw)
	{
	status_text("GHW loaded successfully.\n");
	}
else
if(GLOBALS->is_lx2)
	{
	switch(GLOBALS->is_lx2)
		{
		case LXT2_IS_LXT2: status_text("LXT2 loaded successfully.\n"); break;
		case LXT2_IS_AET2: status_text("AET2 loaded successfully.\n"); break;
		case LXT2_IS_VZT:  status_text("VZT loaded successfully.\n"); break;
		case LXT2_IS_VLIST:  status_text("VCD loaded successfully.\n"); break;
		case LXT2_IS_FSDB:  status_text("FSDB loaded successfully.\n"); break;
		}
	}

sprintf(buf,"[%d] facilities found.\n",GLOBALS->numfacs);
status_text(buf);

if((GLOBALS->is_vcd)||(GLOBALS->is_ghw))
	{
	if(!GLOBALS->partial_vcd)
		{
		sprintf(buf,"[%d] regions found.\n",GLOBALS->regions);
		status_text(buf);
		}
	}
else
	{
	if(GLOBALS->is_lx2 == LXT2_IS_VLIST)
		{
		sprintf(buf,"Regions formed on demand.\n");
		}
		else
		{
		sprintf(buf,"Regions loaded on demand.\n");
		}
	status_text(buf);
	}
}

/* Create a scrolled text area that displays a "message" */
GtkWidget *
create_text (void)
{
GtkWidget *table;

GtkTooltips *tooltips;

tooltips=gtk_tooltips_new_2();
gtk_tooltips_set_delay_2(tooltips,1500);

/* Create a table to hold the text widget and scrollbars */
table = gtk_table_new (1, 16, FALSE);

/* Put a text widget in the upper left hand corner. Note the use of
* GTK_SHRINK in the y direction */
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN)
GLOBALS->text_status_c_2 = gtk_text_view_new ();
gtk_text_view_set_editable (GTK_TEXT_VIEW(GLOBALS->text_status_c_2), FALSE);
gtk_text_buffer_get_start_iter (gtk_text_view_get_buffer(GTK_TEXT_VIEW (GLOBALS->text_status_c_2)), &GLOBALS->iter_status_c_3);
GLOBALS->bold_tag_status_c_3 = gtk_text_buffer_create_tag (GTK_TEXT_VIEW (GLOBALS->text_status_c_2)->buffer, "bold",
                                      "weight", PANGO_WEIGHT_BOLD, NULL);
GtkTextIter iter;
gtk_text_buffer_get_end_iter (gtk_text_view_get_buffer(GTK_TEXT_VIEW (GLOBALS->text_status_c_2)), &iter);
gtk_text_buffer_create_mark (gtk_text_view_get_buffer(GTK_TEXT_VIEW (GLOBALS->text_status_c_2)), "end", &iter, FALSE);
#else
GLOBALS->text_status_c_2 = gtk_text_new (NULL, NULL);
gtk_text_set_editable(GTK_TEXT(GLOBALS->text_status_c_2), FALSE);
#endif
gtk_table_attach (GTK_TABLE (table), GLOBALS->text_status_c_2, 0, 14, 0, 1,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_SHRINK | GTK_EXPAND, 0, 0);
gtk_widget_set_usize(GTK_WIDGET(GLOBALS->text_status_c_2), 100, 50);
gtk_widget_show (GLOBALS->text_status_c_2);

/* And a VScrollbar in the upper right */
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN)
{
GtkTextViewClass *tc = (GtkTextViewClass*)GTK_OBJECT_GET_CLASS(GTK_OBJECT(GLOBALS->text_status_c_2));

tc->set_scroll_adjustments(GTK_TEXT_VIEW (GLOBALS->text_status_c_2), NULL, NULL);
GLOBALS->vscrollbar_status_c_2 = gtk_vscrollbar_new (GTK_TEXT_VIEW (GLOBALS->text_status_c_2)->vadjustment);
}
#else
GLOBALS->vscrollbar_status_c_2 = gtk_vscrollbar_new ((GTK_TEXT (GLOBALS->text_status_c_2))->vadj);
#endif
gtk_table_attach (GTK_TABLE (table), GLOBALS->vscrollbar_status_c_2, 15, 16, 0, 1,
			GTK_FILL, GTK_FILL | GTK_SHRINK | GTK_EXPAND, 0, 0);
gtk_widget_show (GLOBALS->vscrollbar_status_c_2);

/* Add a handler to put a message in the text widget when it is realized */
gtk_signal_connect (GTK_OBJECT (GLOBALS->text_status_c_2), "realize", GTK_SIGNAL_FUNC (realize_text), NULL);

gtk_tooltips_set_tip_2(tooltips, GLOBALS->text_status_c_2, "Status Window", NULL);
return(table);
}


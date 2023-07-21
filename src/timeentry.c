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

if(GLOBALS->from_entry)
	{
	reformat_time(str, GLOBALS->tims.first + GLOBALS->global_time_offset, GLOBALS->time_dimension);
	gtk_entry_set_text(GTK_ENTRY(GLOBALS->from_entry),str);
	gtkwavetcl_setvar(WAVE_TCLCB_FROM_ENTRY_UPDATED, str, WAVE_TCLCB_FROM_ENTRY_UPDATED_FLAGS);
	}

if(GLOBALS->to_entry)
	{
	reformat_time(str, GLOBALS->tims.last + GLOBALS->global_time_offset, GLOBALS->time_dimension);
	gtk_entry_set_text(GTK_ENTRY(GLOBALS->to_entry),str);
	gtkwavetcl_setvar(WAVE_TCLCB_TO_ENTRY_UPDATED, str, WAVE_TCLCB_TO_ENTRY_UPDATED_FLAGS);
	}
}

void time_update(void)
{
DEBUG(printf("Timeentry Configure Event\n"));

calczoom(GLOBALS->tims.zoom);
fix_wavehadj();
g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed");
g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed");
}


void from_entry_callback(GtkWidget *widget, GtkWidget *entry)
{
(void)widget;

G_CONST_RETURN gchar *entry_text;
TimeType newlo;
char fromstr[40];

entry_text=gtk_entry_get_text(GTK_ENTRY(entry));
entry_text = entry_text ? entry_text : "";
DEBUG(printf("From Entry contents: %s\n",entry_text));

newlo=unformat_time(entry_text, GLOBALS->time_dimension);
newlo -= GLOBALS->global_time_offset;

if(newlo<GLOBALS->min_time)
	{
	newlo=GLOBALS->min_time;
	}

if(newlo<(GLOBALS->tims.last))
	{
	GLOBALS->tims.first=newlo;
	if(GLOBALS->tims.start<GLOBALS->tims.first) GLOBALS->tims.timecache=GLOBALS->tims.start=GLOBALS->tims.first;

	reformat_time(fromstr, GLOBALS->tims.first + GLOBALS->global_time_offset, GLOBALS->time_dimension);
	gtk_entry_set_text(GTK_ENTRY(entry),fromstr);
	time_update();
	gtkwavetcl_setvar(WAVE_TCLCB_FROM_ENTRY_UPDATED, fromstr, WAVE_TCLCB_FROM_ENTRY_UPDATED_FLAGS);
	return;
	}
	else
	{
	reformat_time(fromstr, GLOBALS->tims.first + GLOBALS->global_time_offset, GLOBALS->time_dimension);
	gtk_entry_set_text(GTK_ENTRY(entry),fromstr);
	gtkwavetcl_setvar(WAVE_TCLCB_FROM_ENTRY_UPDATED, fromstr, WAVE_TCLCB_FROM_ENTRY_UPDATED_FLAGS);
	return;
	}
}

void to_entry_callback(GtkWidget *widget, GtkWidget *entry)
{
(void)widget;

G_CONST_RETURN gchar *entry_text;
TimeType newhi;
char tostr[40];

entry_text=gtk_entry_get_text(GTK_ENTRY(entry));
entry_text = entry_text ? entry_text : "";
DEBUG(printf("To Entry contents: %s\n",entry_text));

newhi=unformat_time(entry_text, GLOBALS->time_dimension);
newhi -= GLOBALS->global_time_offset;

if((newhi>GLOBALS->max_time) || (!strlen(entry_text))) /* null string makes max time */
	{
	newhi=GLOBALS->max_time;
	}

if(newhi>(GLOBALS->tims.first))
	{
	GLOBALS->tims.last=newhi;
	reformat_time(tostr, GLOBALS->tims.last + GLOBALS->global_time_offset, GLOBALS->time_dimension);
	gtk_entry_set_text(GTK_ENTRY(entry),tostr);
	time_update();
	gtkwavetcl_setvar(WAVE_TCLCB_TO_ENTRY_UPDATED, tostr, WAVE_TCLCB_TO_ENTRY_UPDATED_FLAGS);
	return;
	}
	else
	{
	reformat_time(tostr, GLOBALS->tims.last + GLOBALS->global_time_offset, GLOBALS->time_dimension);
	gtk_entry_set_text(GTK_ENTRY(entry),tostr);
	gtkwavetcl_setvar(WAVE_TCLCB_TO_ENTRY_UPDATED, tostr, WAVE_TCLCB_TO_ENTRY_UPDATED_FLAGS);
	return;
	}
}

/* Create an entry box */
GtkWidget *
create_entry_box(void)
{
GtkWidget *label, *label2;
GtkWidget *box, *box2;
GtkWidget *mainbox;

char fromstr[32], tostr[32];

label=gtk_label_new("From:");
GLOBALS->from_entry=X_gtk_entry_new_with_max_length(40);

reformat_time(fromstr, GLOBALS->min_time + GLOBALS->global_time_offset, GLOBALS->time_dimension);

gtk_entry_set_text(GTK_ENTRY(GLOBALS->from_entry),fromstr);
g_signal_connect (XXX_GTK_OBJECT (GLOBALS->from_entry), "activate",G_CALLBACK (from_entry_callback), GLOBALS->from_entry);
box=XXX_gtk_hbox_new(FALSE, 0);
gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
gtk_widget_show(label);
gtk_box_pack_start(GTK_BOX(box), GLOBALS->from_entry, TRUE, TRUE, 0);
gtk_widget_set_size_request(GTK_WIDGET(GLOBALS->from_entry), 90, 22);
gtk_tooltips_set_tip_2(GLOBALS->from_entry, "Scroll Lower Bound (use MX for named marker X)");
gtk_widget_show(GLOBALS->from_entry);


label2=gtk_label_new("To:");
GLOBALS->to_entry=X_gtk_entry_new_with_max_length(40);

reformat_time(tostr, GLOBALS->max_time + GLOBALS->global_time_offset, GLOBALS->time_dimension);

gtk_entry_set_text(GTK_ENTRY(GLOBALS->to_entry),tostr);
g_signal_connect (XXX_GTK_OBJECT (GLOBALS->to_entry), "activate",G_CALLBACK (to_entry_callback), GLOBALS->to_entry);
box2=XXX_gtk_hbox_new(FALSE, 0);
gtk_box_pack_start(GTK_BOX(box2), label2, TRUE, TRUE, 0);
gtk_widget_show(label2);
gtk_box_pack_start(GTK_BOX(box2), GLOBALS->to_entry, TRUE, TRUE, 0);
gtk_widget_set_size_request(GTK_WIDGET(GLOBALS->to_entry), 90, 22);
gtk_tooltips_set_tip_2(GLOBALS->to_entry, "Scroll Upper Bound (use MX for named marker X)");
gtk_widget_show(GLOBALS->to_entry);

if(!GLOBALS->use_toolbutton_interface)
	{
	mainbox=XXX_gtk_vbox_new(FALSE, 0);
	}
	else
	{
	mainbox=XXX_gtk_hbox_new(FALSE, 0);
	}

gtk_box_pack_start(GTK_BOX(mainbox), box, TRUE, FALSE, 1);
gtk_widget_show(box);
gtk_box_pack_start(GTK_BOX(mainbox), box2, TRUE, FALSE, 1);
gtk_widget_show(box2);

return(mainbox);
}


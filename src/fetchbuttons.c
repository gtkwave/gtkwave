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


void
fetch_left(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

TimeType newlo;
char fromstr[32];

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nFetch Left");
        help_text(
                " decreases the \"From\" time, which allows more of the trace"
		" to be displayed if the \"From\" and \"To\" times do not match"
		" the actual bounds of the trace."
        );
        return;
        }


DEBUG(printf("Fetch Left\n"));

newlo=(GLOBALS->tims.first)-GLOBALS->fetchwindow;

if(newlo<=GLOBALS->min_time) newlo=GLOBALS->min_time;

reformat_time(fromstr, newlo, GLOBALS->time_dimension);

gtk_entry_set_text(GTK_ENTRY(GLOBALS->from_entry),fromstr);

if(newlo<(GLOBALS->tims.last))
        {
        GLOBALS->tims.first=newlo;
        if(GLOBALS->tims.start<GLOBALS->tims.first) GLOBALS->tims.start=GLOBALS->tims.first;

        time_update();
        }
}


void
fetch_right(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

TimeType newhi;
char tostr[32];

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nFetch Right");
        help_text(
                " increases the \"To\" time, which allows more of the trace"
                " to be displayed if the \"From\" and \"To\" times do not match"
                " the actual bounds of the trace."
        );
        return;
        }


DEBUG(printf("Fetch Right\n"));

newhi=(GLOBALS->tims.last)+GLOBALS->fetchwindow;

if(newhi>=GLOBALS->max_time) newhi=GLOBALS->max_time;

reformat_time(tostr, newhi, GLOBALS->time_dimension);

gtk_entry_set_text(GTK_ENTRY(GLOBALS->to_entry),tostr);

if(newhi>(GLOBALS->tims.first))
        {
        GLOBALS->tims.last=newhi;
        time_update();
        }
}


void
discard_left(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

TimeType newlo;
char tostr[32];

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nDiscard Left");
        help_text(
                " increases the \"From\" time, which allows less of the trace"
                " to be displayed."
        );
        return;
        }


DEBUG(printf("Discard Left\n"));

newlo=(GLOBALS->tims.first)+GLOBALS->fetchwindow;

if(newlo<(GLOBALS->tims.last))
	{
	reformat_time(tostr, newlo, GLOBALS->time_dimension);
	gtk_entry_set_text(GTK_ENTRY(GLOBALS->from_entry),tostr);

	GLOBALS->tims.first=newlo;
	time_update();
	}
}

void
discard_right(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

TimeType newhi;
char tostr[32];

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nDiscard Right");
        help_text(
                " decreases the \"To\" time, which allows less of the trace"
                " to be displayed."
        );
        return;
        }


DEBUG(printf("Discard Right\n"));

newhi=(GLOBALS->tims.last)-GLOBALS->fetchwindow;

if(newhi>(GLOBALS->tims.first))
	{
	reformat_time(tostr, newhi, GLOBALS->time_dimension);
	gtk_entry_set_text(GTK_ENTRY(GLOBALS->to_entry),tostr);

	GLOBALS->tims.last=newhi;
	time_update();
	}
}

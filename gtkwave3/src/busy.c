/*
 * Copyright (c) Tony Bybell 2006-2009.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include "busy.h"

static int inside_iteration = 0;

void gtk_events_pending_gtk_main_iteration(void)
{
inside_iteration++;

while (gtk_events_pending()) gtk_main_iteration();

inside_iteration--;
}

gboolean in_main_iteration(void)
{
return(inside_iteration != 0);
}


gboolean ignore_context_swap(void)
{
return(GLOBALS->splash_is_loading != 0);
}


static void GuiDoEvent(GdkEvent *event, gpointer data)
{
(void)data;

if(!GLOBALS->busy_busy_c_1)
	{
	gtk_main_do_event(event);
	}
	else
	{
	/* filter out user input when we're "busy" */

	/* originally we allowed these two sets only... */

        /* usual expose events */
        /* case GDK_CONFIGURE: */
        /* case GDK_EXPOSE: */

        /* needed to keep dnd from hanging */
        /* case GDK_ENTER_NOTIFY: */
        /* case GDK_LEAVE_NOTIFY: */
        /* case GDK_FOCUS_CHANGE: */
        /* case GDK_MAP: */

	/* now it has been updated to remove keyboard/mouse input */

	switch (event->type)
		{
		/* more may be needed to be added in the future */
		case GDK_MOTION_NOTIFY:
		case GDK_BUTTON_PRESS:
		case GDK_2BUTTON_PRESS:
		case GDK_3BUTTON_PRESS:
		case GDK_BUTTON_RELEASE:
		case GDK_KEY_PRESS:
		case GDK_KEY_RELEASE:
#if GTK_CHECK_VERSION(2,6,0)
		case GDK_SCROLL:
#endif
			/* printf("event->type: %d\n", event->type); */
			break;

		default:
	            	gtk_main_do_event(event);
			/* printf("event->type: %d\n", event->type); */
			break;
		}
	}
}


void gtkwave_main_iteration(void)
{
if(GLOBALS->partial_vcd)
	{
	gtk_events_pending_gtk_main_iteration();
	}
	else
	{
	struct Global *g_old = GLOBALS;
	struct Global *gcache = NULL;

	set_window_busy(NULL);

	while (gtk_events_pending())
		{
		gtk_main_iteration();
		if(GLOBALS != g_old)
			{
			/* this should never happen! */
			/* if it does, the program state is probably screwed */
			fprintf(stderr, "GTKWAVE | WARNING: globals changed during gtkwave_main_iteration()!\n");
			gcache = GLOBALS;
			}
		}

	set_GLOBALS(g_old);
	set_window_idle(NULL);

	if(gcache)
		{
		set_GLOBALS(gcache);
		}
	}
}


void init_busy(void)
{
GLOBALS->busycursor_busy_c_1 = gdk_cursor_new(GDK_WATCH);
gdk_event_handler_set((GdkEventFunc)GuiDoEvent, NULL, NULL);
}


void set_window_busy_no_refresh(GtkWidget *w)
{
unsigned int i;

/* if(GLOBALS->tree_dnd_begin) return; */

if(!GLOBALS->busy_busy_c_1)
	{
	if(w) gdk_window_set_cursor (w->window, GLOBALS->busycursor_busy_c_1);
	else
	if(GLOBALS->mainwindow) gdk_window_set_cursor (GLOBALS->mainwindow->window, GLOBALS->busycursor_busy_c_1);
	}

GLOBALS->busy_busy_c_1++;

for(i=0;i<GLOBALS->num_notebook_pages;i++)
        {
        (*GLOBALS->contexts)[i]->busy_busy_c_1 = GLOBALS->busy_busy_c_1;
        }
}

void set_window_busy(GtkWidget *w)
{
set_window_busy_no_refresh(w);
busy_window_refresh();
}

void set_window_idle(GtkWidget *w)
{
unsigned int i;

/* if(GLOBALS->tree_dnd_begin) return; */

if(GLOBALS->busy_busy_c_1)
	{
	if(GLOBALS->busy_busy_c_1 <= 1) /* defensively, in case something causes the value to go below zero */
		{
		if(w) gdk_window_set_cursor (w->window, NULL);
		else
		if(GLOBALS->mainwindow) gdk_window_set_cursor (GLOBALS->mainwindow->window, NULL);
		}

	GLOBALS->busy_busy_c_1--;

	for(i=0;i<GLOBALS->num_notebook_pages;i++)
	        {
	        (*GLOBALS->contexts)[i]->busy_busy_c_1 = GLOBALS->busy_busy_c_1;
	        }
	}
}


void busy_window_refresh(void)
{
if(GLOBALS->busy_busy_c_1)
	{
	gtk_events_pending_gtk_main_iteration();
	}
}


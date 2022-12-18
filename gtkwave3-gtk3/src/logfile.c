/*
 * Copyright (c) Tony Bybell 1999-2010.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include <ctype.h>
#include "debug.h"
#include "symbol.h"
#include "currenttime.h"
#include "fgetdynamic.h"

/* only for use locally */
struct wave_logfile_lines_t
{
struct wave_logfile_lines_t *next;
char *text;
};

struct logfile_instance_t
{
struct logfile_instance_t *next;
GtkWidget *window;
GtkWidget *text;

GtkTextTag *bold_tag;
GtkTextTag *mono_tag;
GtkTextTag *size_tag;

char default_text[1];
};

#define log_collection (*((struct logfile_instance_t **)GLOBALS->logfiles))


/* Add some text to our text widget - this is a callback that is invoked
when our window is realized. We could also force our window to be
realized with gtk_widget_realize, but it would have to be part of
a hierarchy first */


void log_text(GtkWidget *text, void *font, char *str)
{
(void)font;

gtk_text_buffer_insert_with_tags (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), &GLOBALS->iter_logfile_c_2,
                                 str, -1, GLOBALS->mono_tag_logfile_c_1, GLOBALS->size_tag_logfile_c_1, NULL);
}

void log_text_bold(GtkWidget *text, void *font, char *str)
{
(void)font;

gtk_text_buffer_insert_with_tags (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), &GLOBALS->iter_logfile_c_2,
                                 str, -1, GLOBALS->bold_tag_logfile_c_2, GLOBALS->mono_tag_logfile_c_1, GLOBALS->size_tag_logfile_c_1, NULL);
}

static void
log_realize_text (GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

/* nothing for now */
}


static void center_op(void)
{
TimeType middle=0, width;

if((GLOBALS->tims.marker<0)||(GLOBALS->tims.marker<GLOBALS->tims.first)||(GLOBALS->tims.marker>GLOBALS->tims.last))
	{
        if(GLOBALS->tims.end>GLOBALS->tims.last) GLOBALS->tims.end=GLOBALS->tims.last;
        middle=(GLOBALS->tims.start/2)+(GLOBALS->tims.end/2);
        if((GLOBALS->tims.start&1)&&(GLOBALS->tims.end&1)) middle++;
        }
        else
        {
        middle=GLOBALS->tims.marker;
        }

width=(TimeType)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);
GLOBALS->tims.start=time_trunc(middle-(width/2));
if(GLOBALS->tims.start+width>GLOBALS->tims.last) GLOBALS->tims.start=time_trunc(GLOBALS->tims.last-width);
if(GLOBALS->tims.start<GLOBALS->tims.first) GLOBALS->tims.start=GLOBALS->tims.first;
gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider), GLOBALS->tims.timecache=GLOBALS->tims.start);

fix_wavehadj();

g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */
}



static gboolean
button_release_event (GtkWidget *text, GdkEventButton *event)
{
(void)event;

gchar *sel;

GtkTextIter start;
GtkTextIter end;

if (gtk_text_buffer_get_selection_bounds (gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                         &start, &end))
       {
       if(gtk_text_iter_compare (&start, &end) < 0)
               {
               sel = gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                              &start, &end, FALSE);

               if(sel)
                       {
			int slen = strlen(sel);
			char *sel2 = NULL;

                       	if((slen)&&(sel[0]>='0')&&(sel[0]<='9'))
                               {
			       TimeType tm;
			       gunichar gch = gtk_text_iter_get_char(&end);
			       int do_si_append = 0;

			        if(gch==' ') /* in case time is of format "100 ps" with a space */
					{
					gtk_text_iter_forward_char(&end);
					gch = gtk_text_iter_get_char(&end);
					}

				if((sel[slen-1]>='0')&&(sel[slen-1]<='9')) /* need to append units? */
					{
					int silen = strlen(WAVE_SI_UNITS);
					int silp;

					gch = tolower(gch);
					if(gch == 's')
						{
						do_si_append = 1;
						}
					else
						{
						for(silp=0;silp<silen;silp++)
							{
							if((unsigned)gch == (unsigned)WAVE_SI_UNITS[silp])
								{
								do_si_append = 1;
								break;
								}
							}
						}
					}

				if(do_si_append)
					{
					sel2 = malloc_2(slen + 2);
					sprintf(sel2, "%s%c", sel, (unsigned char)gch);
					}

                               tm = unformat_time(sel2 ? sel2 : sel, GLOBALS->time_dimension);
                               if((tm >= GLOBALS->tims.first) && (tm <= GLOBALS->tims.last))
                                       {
                                       GLOBALS->tims.lmbcache = -1;
                                       update_markertime(GLOBALS->tims.marker = tm);
                                       center_op();
                                       signalarea_configure_event(GLOBALS->signalarea, NULL);
                                       wavearea_configure_event(GLOBALS->wavearea, NULL);
                                       update_markertime(GLOBALS->tims.marker = tm); /* centering problem in GTK2 */
                                       }
                               }

		       if(sel2) { free_2(sel2); }
                       g_free(sel);
                       }
               }
       }

return(FALSE); /* call remaining handlers... */
}

/* Create a scrolled text area that displays a "message" */
static GtkWidget *create_log_text (GtkWidget **textpnt)
{
GtkWidget *text;
GtkWidget *scrolled_window;

text = gtk_text_view_new ();
gtk_text_buffer_get_start_iter (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), &GLOBALS->iter_logfile_c_2);
GLOBALS->bold_tag_logfile_c_2 = gtk_text_buffer_create_tag (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), "bold",
                                      "weight", PANGO_WEIGHT_BOLD, NULL);
GLOBALS->mono_tag_logfile_c_1 = gtk_text_buffer_create_tag (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), "monospace", "family", "monospace", NULL);

GLOBALS->size_tag_logfile_c_1 = gtk_text_buffer_create_tag (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), "fsiz",
			      "size", (GLOBALS->use_big_fonts ? 12 : 8) * PANGO_SCALE, NULL);


*textpnt = text;
gtk_widget_set_size_request(GTK_WIDGET(text), 100, 100);
gtk_text_view_set_editable(GTK_TEXT_VIEW(text), TRUE);
gtk_widget_show (text);

scrolled_window = gtk_scrolled_window_new (NULL, NULL);
gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
gtk_container_add (GTK_CONTAINER (scrolled_window), text);
gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 5);
gtk_widget_show(scrolled_window);

/* Add a handler to put a message in the text widget when it is realized */
g_signal_connect (XXX_GTK_OBJECT (text), "realize", G_CALLBACK (log_realize_text), NULL);

g_signal_connect(XXX_GTK_OBJECT(text), "button_release_event", G_CALLBACK(button_release_event), NULL);

gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_CHAR);
return(scrolled_window);
}

/***********************************************************************************/

static void ok_callback(GtkWidget *widget, GtkWidget *cached_window)
{
(void)widget;

struct logfile_instance_t *l = log_collection;
struct logfile_instance_t *lprev = NULL;

while(l)
	{
	if(l->window == cached_window)
		{
		if(lprev)
			{
			lprev->next = l->next;
			}
			else
			{
			log_collection = l->next;
			}

		free(l);  /* deliberately not free_2 */
		break;
		}

	lprev = l;
	l = l->next;
	}

  DEBUG(printf("OK\n"));
  gtk_widget_destroy(cached_window);
}


static void destroy_callback(GtkWidget *widget, GtkWidget *cached_window)
{
(void)cached_window;

ok_callback(widget, widget);
}


void logbox(char *title, int width, char *default_text)
{
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *hbox, *button1;
    GtkWidget *label, *separator;
    GtkWidget *ctext;
    GtkWidget *text;
    struct logfile_instance_t *log_c;

    FILE *handle;
    struct wave_logfile_lines_t *wlog_head=NULL, *wlog_curr=NULL;
    int wlog_size = 0;

    handle = fopen(default_text, "rb");
    if(!handle)
	{
	char *buf = malloc_2(strlen(default_text)+128);
	sprintf(buf, "Could not open logfile '%s'\n", default_text);
	status_text(buf);
	free_2(buf);
	return;
	}

    /* fix problem where ungrab doesn't occur if button pressed + simultaneous accelerator key occurs */
    if(GLOBALS->in_button_press_wavewindow_c_1) { XXX_gdk_pointer_ungrab(GDK_CURRENT_TIME); }

/* nothing */

    /* create a new nonmodal window */
    window = gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    if(GLOBALS->use_big_fonts || GLOBALS->fontname_logfile)
	{
    	gtk_widget_set_size_request( GTK_WIDGET (window), width*1.8, 600);
	}
	else
	{
    	gtk_widget_set_size_request( GTK_WIDGET (window), width, 400);
	}
    gtk_window_set_title(GTK_WINDOW (window), title);

    g_signal_connect(XXX_GTK_OBJECT (window), "delete_event", (GCallback) destroy_callback, window);

    vbox = XXX_gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_widget_show (vbox);

    label=gtk_label_new(default_text);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    separator = XXX_gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 0);
    gtk_widget_show (separator);

    ctext=create_log_text(&text);
    gtk_box_pack_start (GTK_BOX (vbox), ctext, TRUE, TRUE, 0);
    gtk_widget_show (ctext);

    separator = XXX_gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 0);
    gtk_widget_show (separator);

    hbox = XXX_gtk_hbox_new (FALSE, 1);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label ("Close Logfile");
    gtk_widget_set_size_request(button1, 100, -1);
    g_signal_connect(XXX_GTK_OBJECT (button1), "clicked", G_CALLBACK(ok_callback), window);
    gtk_widget_show (button1);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_box_pack_start(GTK_BOX(hbox), button1, TRUE, TRUE, 0);
#else
    gtk_container_add (GTK_CONTAINER (hbox), button1);
#endif
    gtk_widget_set_can_default (button1, TRUE);
    g_signal_connect_swapped (XXX_GTK_OBJECT (button1), "realize", (GCallback) gtk_widget_grab_default, XXX_GTK_OBJECT (button1));

    gtk_widget_show(window);

    log_text_bold(text, NULL, "Click-select");
    log_text(text, NULL, " on numbers to jump to that time value in the wave viewer.\n");
    log_text(text, NULL, " \n");

    while(!feof(handle))
	{
	char *pnt = fgetmalloc(handle);
	if(pnt)
		{
		struct wave_logfile_lines_t *w = calloc_2(1, sizeof(struct wave_logfile_lines_t));

		wlog_size += (GLOBALS->fgetmalloc_len+1);
		w->text = pnt;
		if(!wlog_curr) { wlog_head = wlog_curr = w; } else { wlog_curr->next = w; wlog_curr = w; }
		}
	}

    if(wlog_curr)
	{
	struct wave_logfile_lines_t *w = wlog_head;
	struct wave_logfile_lines_t *wt;
	char *pnt = malloc_2(wlog_size + 1);
	char *pnt2 = pnt;

	while(w)
		{
		int len = strlen(w->text);
		memcpy(pnt2, w->text, len);
		pnt2 += len;
		*pnt2 = '\n';
		pnt2++;

		free_2(w->text);
		wt = w;
		w = w->next;
		free_2(wt);
		}
	/* wlog_head = */ wlog_curr = NULL; /* scan-build */
	*pnt2 = 0;
	log_text(text, NULL, pnt);
	free_2(pnt);
	}

    fclose(handle);

    log_c = calloc(1, sizeof(struct logfile_instance_t) + strlen(default_text));  /* deliberately not calloc_2, needs to be persistent! */
    strcpy(log_c->default_text, default_text);
    log_c->window = window;
    log_c->text = text;
    log_c->next = log_collection;
    log_c->bold_tag = GLOBALS->bold_tag_logfile_c_2;
    log_c->mono_tag = GLOBALS->mono_tag_logfile_c_1;
    log_c->size_tag = GLOBALS->size_tag_logfile_c_1;
    log_collection = log_c;
}


static void logbox_reload_single(GtkWidget *window, GtkWidget *text, char *default_text)
{
(void)window;

    FILE *handle;
    struct wave_logfile_lines_t *wlog_head=NULL, *wlog_curr=NULL;
    int wlog_size = 0;

    handle = fopen(default_text, "rb");
    if(!handle)
	{
	char *buf = malloc_2(strlen(default_text)+128);
	sprintf(buf, "Could not open logfile '%s'\n", default_text);
	status_text(buf);
	free_2(buf);
	return;
	}

    {
    GtkTextIter st_iter, en_iter;

    gtk_text_buffer_get_start_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), &st_iter);
    gtk_text_buffer_get_end_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), &en_iter);
    gtk_text_buffer_delete(gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), &st_iter, &en_iter);

    gtk_text_buffer_get_start_iter (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), &GLOBALS->iter_logfile_c_2);
    }

    log_text_bold(text, NULL, "Click-select");
    log_text(text, NULL, " on numbers to jump to that time value in the wave viewer.\n");
    log_text(text, NULL, " \n");

    while(!feof(handle))
	{
	char *pnt = fgetmalloc(handle);
	if(pnt)
		{
		struct wave_logfile_lines_t *w = calloc_2(1, sizeof(struct wave_logfile_lines_t));

		wlog_size += (GLOBALS->fgetmalloc_len+1);
		w->text = pnt;
		if(!wlog_curr) { wlog_head = wlog_curr = w; } else { wlog_curr->next = w; wlog_curr = w; }
		}
	}

    if(wlog_curr)
	{
	struct wave_logfile_lines_t *w = wlog_head;
	struct wave_logfile_lines_t *wt;
	char *pnt = malloc_2(wlog_size + 1);
	char *pnt2 = pnt;

	while(w)
		{
		int len = strlen(w->text);
		memcpy(pnt2, w->text, len);
		pnt2 += len;
		*pnt2 = '\n';
		pnt2++;

		free_2(w->text);
		wt = w;
		w = w->next;
		free_2(wt);
		}
	/* wlog_head = */ wlog_curr = NULL; /* scan-build */
	*pnt2 = 0;
	log_text(text, NULL, pnt);
	free_2(pnt);
	}


    fclose(handle);
}


void logbox_reload(void)
{
struct logfile_instance_t *l = log_collection;

while(l)
	{
	GLOBALS->bold_tag_logfile_c_2 = l->bold_tag;
	GLOBALS->mono_tag_logfile_c_1 = l->mono_tag;
	GLOBALS->size_tag_logfile_c_1 = l->size_tag;

	logbox_reload_single(l->window, l->text, l->default_text);
	l = l->next;
	}
}


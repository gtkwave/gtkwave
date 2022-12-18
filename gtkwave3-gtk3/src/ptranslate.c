/*
 * Copyright (c) Tony Bybell 2005-2009.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include "gtk23compat.h"
#include "symbol.h"
#include "ptranslate.h"
#include "pipeio.h"
#include "debug.h"


enum { NAME_COLUMN, N_COLUMNS };

static gboolean
  XXX_view_selection_func (GtkTreeSelection *selection,
                       GtkTreeModel     *model,
                       GtkTreePath	*path,
                       gboolean          path_currently_selected,
                       gpointer          userdata)
{
(void) selection;
(void) model;
(void) userdata;

gint *idx = NULL;

if(path)
        {
	idx = gtk_tree_path_get_indices (path);

        if(idx)
               	{
                if(!path_currently_selected)
                        {
                        GLOBALS->current_filter_ptranslate_c_1 = idx[0] + 1;
                        }
                        else
                        {
                        GLOBALS->current_filter_ptranslate_c_1 = 0; /* none */
                        }
                }
        }

return(TRUE);
}


void init_proctrans_data(void)
{
int i;

if(!GLOBALS->procsel_filter) { GLOBALS->procsel_filter = calloc_2(PROC_FILTER_MAX+1, sizeof(char *)); }
if(!GLOBALS->proc_filter) { GLOBALS->proc_filter = calloc_2(PROC_FILTER_MAX+1, sizeof(struct pipe_ctx *)); }

for(i=0;i<PROC_FILTER_MAX+1;i++)
	{
	GLOBALS->procsel_filter[i] = NULL;
	GLOBALS->proc_filter[i] = NULL;
	}
}

void remove_all_proc_filters(void)
{
struct Global *GLOBALS_cache = GLOBALS;
unsigned int i, j;

for(j=0;j<GLOBALS->num_notebook_pages;j++)
	{
	GLOBALS = (*GLOBALS->contexts)[j];

	if(GLOBALS)
		{
		for(i=1;i<PROC_FILTER_MAX+1;i++)
			{
			if(GLOBALS->proc_filter[i])
				{
				pipeio_destroy(GLOBALS->proc_filter[i]);
				GLOBALS->proc_filter[i] = NULL;
				}

			if(GLOBALS->procsel_filter[i])
				{
				free_2(GLOBALS->procsel_filter[i]);
				GLOBALS->procsel_filter[i] = NULL;
				}
			}
		}

	GLOBALS = GLOBALS_cache;
	}
}


static void regen_display(void)
{
GLOBALS->signalwindow_width_dirty=1;
MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}


/*
 * this is likely obsolete
 */
#if 0
static void remove_proc_filter(int which, int regen)
{
if(GLOBALS->proc_filter[which])
	{
	pipeio_destroy(GLOBALS->proc_filter[which]);
	GLOBALS->proc_filter[which] = NULL;

	if(regen)
	        {
		regen_display();
	        }
	}
}
#endif

static void load_proc_filter(int which, char *name)
{

  FILE *stream;

  char *cmd;
  char exec_name[1025];
  char abs_path [1025];
  char* arg, end;
  int result;

  exec_name[0] = 0;
  abs_path[0]  = 0;

  /* if name has arguments grab only the first word (the name of the executable)*/
  sscanf(name, "%s ", exec_name);

  arg = name + strlen(exec_name);

  /* remove leading spaces from argument */
  while (isspace((int)(unsigned char)arg[0])) {
    arg++;
  }

  /* remove trailing spaces from argument */
  if (strlen(arg) > 0) {

    end = strlen(arg) - 1;

    while (arg[(int)end] == ' ') {
      arg[(int)end] = 0;
      end--;
    }
  }

  /* turn the exec_name into an absolute path */
#if !defined __MINGW32__
  cmd = (char *)malloc_2(strlen(exec_name)+6+1);
  sprintf(cmd, "which %s", exec_name);
  stream = popen(cmd, "r");

  result = fscanf(stream, "%s", abs_path);

  if((strlen(abs_path) == 0)||(!result))
    {
      status_text("Could not find filter process!\n");
      pclose(stream); /* cppcheck */
      return;
    }

  pclose(stream);
  free_2(cmd);
#else
  strcpy(abs_path, exec_name);
#endif


  /* remove_proc_filter(which, 0); ... should never happen from GUI, but perhaps possible from save files or other weirdness */
  if(!GLOBALS->ttrans_filter[which])
	{
	GLOBALS->proc_filter[which] = pipeio_create(abs_path, arg);
	}
}

int install_proc_filter(int which)
{
int found = 0;

if((which<0)||(which>=(PROC_FILTER_MAX+1)))
        {
        which = 0;
        }

if(GLOBALS->traces.first)
        {
        Trptr t = GLOBALS->traces.first;
        while(t)
                {
                if(t->flags&TR_HIGHLIGHT)
                        {
                        if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
                                {
				t->f_filter = 0;
                                t->p_filter = which;
				if(!which)
					{
					t->flags &= (~(TR_FTRANSLATED|TR_PTRANSLATED|TR_ANALOGMASK));
					}
					else
					{
					t->flags &= (~(TR_ANALOGMASK));
					t->flags |= TR_PTRANSLATED;
					}
                                found++;
                                }
                        }
                t=t->t_next;
                }
        }

if(found)
	{
	regen_display();
	}

return(found);
}

/************************************************************************/



static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

GLOBALS->is_active_ptranslate_c_2=0;
gtk_widget_destroy(GLOBALS->window_ptranslate_c_5);
GLOBALS->window_ptranslate_c_5 = NULL;
}

static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
install_proc_filter(GLOBALS->current_filter_ptranslate_c_1);
destroy_callback(widget, nothing);
}

static void add_filter_callback_2(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

int i;

if(!GLOBALS->filesel_ok) { return; }

if(*GLOBALS->fileselbox_text)
	{
	for(i=0;i<GLOBALS->num_proc_filters;i++)
		{
		if(GLOBALS->procsel_filter[i])
			{
			if(!strcmp(GLOBALS->procsel_filter[i], *GLOBALS->fileselbox_text))
				{
				status_text("Filter already imported.\n");
				if(GLOBALS->is_active_ptranslate_c_2) gdk_window_raise(gtk_widget_get_window(GLOBALS->window_ptranslate_c_5));
				return;
				}
			}
		}
	}

GLOBALS->num_proc_filters++;
load_proc_filter(GLOBALS->num_proc_filters, *GLOBALS->fileselbox_text);
if(GLOBALS->proc_filter[GLOBALS->num_proc_filters])
	{
	if(GLOBALS->procsel_filter[GLOBALS->num_proc_filters]) free_2(GLOBALS->procsel_filter[GLOBALS->num_proc_filters]);
	GLOBALS->procsel_filter[GLOBALS->num_proc_filters] = malloc_2(strlen(*GLOBALS->fileselbox_text) + 1);
	strcpy(GLOBALS->procsel_filter[GLOBALS->num_proc_filters], *GLOBALS->fileselbox_text);

        GtkTreeIter iter;
        gtk_list_store_append (GTK_LIST_STORE(GLOBALS->sig_store_ptranslate), &iter);
        gtk_list_store_set (GTK_LIST_STORE(GLOBALS->sig_store_ptranslate), &iter,
                                    NAME_COLUMN, GLOBALS->procsel_filter[GLOBALS->num_proc_filters],
                                    -1);
	}
	else
	{
	GLOBALS->num_proc_filters--;
	}

if(GLOBALS->is_active_ptranslate_c_2) gdk_window_raise(gtk_widget_get_window(GLOBALS->window_ptranslate_c_5));
}

static void add_filter_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

if(GLOBALS->num_proc_filters == PROC_FILTER_MAX)
	{
	status_text("Max number of process filters installed already.\n");
	return;
	}

fileselbox("Select Filter Process",&GLOBALS->fcurr_ptranslate_c_1,G_CALLBACK(add_filter_callback_2), G_CALLBACK(NULL),"*", 0);
}

/*
 * mainline..
 */
void ptrans_searchbox(char *title)
{
    int i;

    GtkWidget *scrolled_win;
    GtkWidget *vbox1, *hbox, *hbox0;
    GtkWidget *button1, *button5, *button6;
    gchar *titles[]={"Process Filter Select"};
    GtkWidget *frame2, *frameh, *frameh0;
    GtkWidget *table;

    if(GLOBALS->is_active_ptranslate_c_2)
	{
	gdk_window_raise(gtk_widget_get_window(GLOBALS->window_ptranslate_c_5));
	return;
	}

    GLOBALS->is_active_ptranslate_c_2=1;
    GLOBALS->current_filter_ptranslate_c_1 = 0;

    /* create a new modal window */
    GLOBALS->window_ptranslate_c_5 = gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window_ptranslate_c_5, ((char *)&GLOBALS->window_ptranslate_c_5) - ((char *)GLOBALS));

    gtk_window_set_title(GTK_WINDOW (GLOBALS->window_ptranslate_c_5), title);
    gtkwave_signal_connect(XXX_GTK_OBJECT (GLOBALS->window_ptranslate_c_5), "delete_event",(GCallback) destroy_callback, NULL);

    table = XXX_gtk_table_new (256, 1, FALSE);
    gtk_widget_show (table);

    vbox1 = XXX_gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox1), 3);
    gtk_widget_show (vbox1);


    frame2 = gtk_frame_new (NULL);
    gtk_container_set_border_width (GTK_CONTAINER (frame2), 3);
    gtk_widget_show(frame2);

    XXX_gtk_table_attach (XXX_GTK_TABLE (table), frame2, 0, 1, 0, 254,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);

    GLOBALS->sig_store_ptranslate = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING);
    GtkWidget *sig_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (GLOBALS->sig_store_ptranslate));

    /* The view now holds a reference.  We can get rid of our own reference */
    g_object_unref (G_OBJECT (GLOBALS->sig_store_ptranslate));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (titles[0], renderer, "text", NAME_COLUMN, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (sig_view), column);

    /* Setup the selection handler */
    GLOBALS->sig_selection_ptranslate = gtk_tree_view_get_selection (GTK_TREE_VIEW (sig_view));
    gtk_tree_selection_set_mode (GLOBALS->sig_selection_ptranslate, GTK_SELECTION_SINGLE);
    gtk_tree_selection_set_select_function (GLOBALS->sig_selection_ptranslate, XXX_view_selection_func, NULL, NULL);

    gtk_list_store_clear (GTK_LIST_STORE(GLOBALS->sig_store_ptranslate));
    for(i=0;i<GLOBALS->num_proc_filters;i++)
        {
	GtkTreeIter iter;
        gtk_list_store_append (GTK_LIST_STORE(GLOBALS->sig_store_ptranslate), &iter);
        gtk_list_store_set (GTK_LIST_STORE(GLOBALS->sig_store_ptranslate), &iter,
                                    NAME_COLUMN, GLOBALS->procsel_filter[i+1],
                                    -1);
        }

    gtk_widget_show (sig_view);

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request( GTK_WIDGET (scrolled_win), -1, 300);
    gtk_widget_show(scrolled_win);

    /* gtk_scrolled_window_add_with_viewport doesn't seen to work right here.. */
    gtk_container_add (GTK_CONTAINER (scrolled_win), sig_view);

    gtk_container_add (GTK_CONTAINER (frame2), scrolled_win);


    frameh0 = gtk_frame_new (NULL);
    gtk_container_set_border_width (GTK_CONTAINER (frameh0), 3);
    gtk_widget_show(frameh0);
    XXX_gtk_table_attach (XXX_GTK_TABLE (table), frameh0, 0, 1, 254, 255,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);


    hbox0 = XXX_gtk_hbox_new (FALSE, 1);
    gtk_widget_show (hbox0);

    button6 = gtk_button_new_with_label (" Add Proc Filter to List ");
    gtk_container_set_border_width (GTK_CONTAINER (button6), 3);
    gtkwave_signal_connect_object (XXX_GTK_OBJECT (button6), "clicked",G_CALLBACK(add_filter_callback),XXX_GTK_OBJECT (GLOBALS->window_ptranslate_c_5));
    gtk_widget_show (button6);
    gtk_tooltips_set_tip_2(button6,
		"Bring up a file requester to add a process filter to the filter select window.");

    gtk_box_pack_start (GTK_BOX (hbox0), button6, TRUE, FALSE, 0);
    gtk_container_add (GTK_CONTAINER (frameh0), hbox0);

    frameh = gtk_frame_new (NULL);
    gtk_container_set_border_width (GTK_CONTAINER (frameh), 3);
    gtk_widget_show(frameh);
    XXX_gtk_table_attach (XXX_GTK_TABLE (table), frameh, 0, 1, 255, 256,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);


    hbox = XXX_gtk_hbox_new (FALSE, 1);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label (" OK ");
    gtk_container_set_border_width (GTK_CONTAINER (button1), 3);
    gtkwave_signal_connect_object (XXX_GTK_OBJECT (button1), "clicked",G_CALLBACK(ok_callback),XXX_GTK_OBJECT (GLOBALS->window_ptranslate_c_5));
    gtk_widget_show (button1);
    gtk_tooltips_set_tip_2(button1,
		"Add selected signals to end of the display on the main window.");

#if GTK_CHECK_VERSION(3,0,0)
    gtk_box_pack_start(GTK_BOX(hbox), button1, TRUE, TRUE, 0);
#else
    gtk_box_pack_start (GTK_BOX (hbox), button1, TRUE, FALSE, 0);
#endif

    button5 = gtk_button_new_with_label (" Cancel ");
    gtk_container_set_border_width (GTK_CONTAINER (button5), 3);
    gtkwave_signal_connect_object (XXX_GTK_OBJECT (button5), "clicked",G_CALLBACK(destroy_callback),XXX_GTK_OBJECT (GLOBALS->window_ptranslate_c_5));
    gtk_tooltips_set_tip_2(button5,
		"Do nothing and return to the main window.");
    gtk_widget_show (button5);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_box_pack_start(GTK_BOX(hbox), button5, TRUE, TRUE, 0);
#else
    gtk_box_pack_start (GTK_BOX (hbox), button5, TRUE, FALSE, 0);
#endif

    gtk_container_add (GTK_CONTAINER (frameh), hbox);
    gtk_container_add (GTK_CONTAINER (GLOBALS->window_ptranslate_c_5), table);

    gtk_widget_set_size_request(GTK_WIDGET(GLOBALS->window_ptranslate_c_5), 400, 400);
    gtk_widget_show(GLOBALS->window_ptranslate_c_5);
}


/*
 * currently only called by parsewavline
 */
void set_current_translate_proc(char *name)
{
int i;

for(i=1;i<GLOBALS->num_proc_filters+1;i++)
	{
	if(!strcmp(GLOBALS->procsel_filter[i], name)) { GLOBALS->current_translate_proc = i; return; }
	}

if(GLOBALS->num_proc_filters < PROC_FILTER_MAX)
	{
	GLOBALS->num_proc_filters++;
	load_proc_filter(GLOBALS->num_proc_filters, name);
	if(!GLOBALS->proc_filter[GLOBALS->num_proc_filters])
		{
		GLOBALS->num_proc_filters--;
		GLOBALS->current_translate_proc = 0;
		}
		else
		{
		if(GLOBALS->procsel_filter[GLOBALS->num_proc_filters]) free_2(GLOBALS->procsel_filter[GLOBALS->num_proc_filters]);
		GLOBALS->procsel_filter[GLOBALS->num_proc_filters] = malloc_2(strlen(name) + 1);
		strcpy(GLOBALS->procsel_filter[GLOBALS->num_proc_filters], name);
		GLOBALS->current_translate_proc = GLOBALS->num_proc_filters;
		}
	}
}



/*
 * Copyright (c) Tony Bybell 1999-2008.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include <gtk/gtk.h>
#include "splay.h"
#include "wavelink.h"

#define set_winsize(w,x,y) gtk_window_set_default_size(GTK_WINDOW(w),(x),(y))
void create_toolbar(GtkWidget *table);

GtkWidget *notebook = NULL;

extern GtkTreeStore *treestore_main;
extern GtkWidget *treeview_main;

void bwmaketree(void);
void bwlogbox(char *title, int width, ds_Tree *t, int display_mode);
void setup_dnd(GtkWidget *wid);

static ds_Tree *selectedtree=NULL;

static int is_active=0;

int treebox_is_active(void)
{
return(is_active);
}

static void XXX_select_row_callback(
GtkTreeModel *model,
GtkTreePath  *path)
{
GtkTreeIter   iter;
ds_Tree *t = NULL;

if (!gtk_tree_model_get_iter(model, &iter, path))
        {
	return; /* path describes a non-existing row - should not happen */
        }

gtk_tree_model_get_iter(model, &iter, path);
gtk_tree_model_get(model, &iter, XXX2_TREE_COLUMN, &t, -1);

if(t && (selectedtree!=t))
	{
	if(t->filename)
	        {
	        /*
	        printf("%s\n", t->fullname);
	        printf("%s -> '%s' %d-%d\n\n", t->item, t->filename, t->s_line, t->e_line);
	        */
	
	        bwlogbox(t->fullname ? t->fullname : "*", 640 + 8*8, t, 0);
	        }
	        else
	        {
	        /*
		printf("%s\n", t->fullname);
	        printf("%s -> *MISSING*\n\n", t->item);
	        */
	        }
	}

selectedtree=t;
}

static void XXX_unselect_row_callback(
GtkTreeModel *model,
GtkTreePath  *path)
{
GtkTreeIter   iter;
ds_Tree *t = NULL;

if (!gtk_tree_model_get_iter(model, &iter, path))
        {
	return; /* path describes a non-existing row - should not happen */
        }

gtk_tree_model_get_iter(model, &iter, path);
gtk_tree_model_get(model, &iter, XXX2_TREE_COLUMN, &t, -1);

/* selectedtree=NULL; */
}

static gboolean
  XXX_view_selection_func (GtkTreeSelection *selection,
                       GtkTreeModel     *model,
                       GtkTreePath	*path,
                       gboolean          path_currently_selected,
                       gpointer          userdata)
{
(void) selection;
(void) userdata;

if(!path_currently_selected)
        {
        XXX_select_row_callback(model, path);
        }
	else
        {
        XXX_unselect_row_callback(model, path);
        }

return(TRUE);
}


/***************************************************************************/

static GtkWidget *window;
static GCallback cleanup;


static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

  is_active=0;
  gtk_widget_destroy(window);
  gtk_main_quit();
}



/*
 * mainline..
 */
void treebox(char *title, GCallback func, GtkWidget *old_window)
{
(void)old_window;

    GtkWidget *scrolled_win;
    GtkWidget *frame2;
    GtkWidget *table;

    if(is_active)
	{
	gdk_window_raise(gtk_widget_get_window(window));
	return;
	}

    is_active=1;
    cleanup=func;

    /* create a new modal window */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW (window), title);
    g_signal_connect(XXX_GTK_OBJECT (window), "delete_event",
                       (GCallback) destroy_callback, NULL);
    set_winsize(window, 640, 600);

#if GTK_CHECK_VERSION(3,0,0)
    table = gtk_grid_new ();
#else
    table = gtk_table_new (256, 1, FALSE);
#endif
    gtk_widget_show (table);

#if GTK_CHECK_VERSION(3,0,0)
    frame2 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
#else
    frame2 = gtk_hpaned_new();
#endif
    gtk_widget_show(frame2);

    notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), ~0);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), ~0);
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), ~0);
    gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));

    gtk_widget_show(notebook);
    gtk_paned_pack2(GTK_PANED(frame2), notebook, TRUE, TRUE);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach (GTK_GRID (table), frame2, 0, 0, 1, 255);
    gtk_widget_set_hexpand (GTK_WIDGET (frame2), TRUE);
    gtk_widget_set_vexpand (GTK_WIDGET (frame2), TRUE);
#else
    gtk_table_attach (GTK_TABLE (table), frame2, 0, 1, 0, 255,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
#endif

    bwmaketree();
    selectedtree=NULL;

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_size_request( GTK_WIDGET (scrolled_win), 150, 300);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_show(scrolled_win);
    gtk_container_add (GTK_CONTAINER (scrolled_win), GTK_WIDGET (treeview_main));
    gtk_tree_selection_set_select_function (gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview_main)),
                                                XXX_view_selection_func, NULL, NULL);
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview_main)), GTK_SELECTION_SINGLE);


    gtk_paned_pack1(GTK_PANED(frame2), scrolled_win, TRUE, TRUE);

    create_toolbar(table);

    gtk_container_add (GTK_CONTAINER (window), table);

    gtk_widget_show(window);
    setup_dnd(window);
}


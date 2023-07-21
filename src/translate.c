/*
 * Copyright (c) Tony Bybell 2005-2014.
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
#include "translate.h"
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
			GLOBALS->current_filter_translate_c_2 = idx[0] + 1;
		        }
			else
		        {
			GLOBALS->current_filter_translate_c_2 = 0; /* none */
		        }
		}
	}

return(TRUE);
}


/************************ splay ************************/

xl_Tree * xl_splay (char *i, xl_Tree * t) {
/* Simple top down splay, not requiring i to be in the tree t.  */
/* What it does is described above.                             */
    xl_Tree N, *l, *r, *y;
    int dir;

    if (t == NULL) return t;
    N.left = N.right = NULL;
    l = r = &N;

    for (;;) {
	dir = strcasecmp(i, t->item);
	if (dir < 0) {
	    if (t->left == NULL) break;
	    if (strcasecmp(i, t->left->item)<0) {
		y = t->left;                           /* rotate right */
		t->left = y->right;
		y->right = t;
		t = y;
		if (t->left == NULL) break;
	    }
	    r->left = t;                               /* link right */
	    r = t;
	    t = t->left;
	} else if (dir > 0) {
	    if (t->right == NULL) break;
	    if (strcasecmp(i, t->right->item)>0) {
		y = t->right;                          /* rotate left */
		t->right = y->left;
		y->left = t;
		t = y;
		if (t->right == NULL) break;
	    }
	    l->right = t;                              /* link left */
	    l = t;
	    t = t->right;
	} else {
	    break;
	}
    }
    l->right = t->left;                                /* assemble */
    r->left = t->right;
    t->left = N.right;
    t->right = N.left;
    return t;
}


xl_Tree * xl_insert(char *i, xl_Tree * t, char *trans) {
/* Insert i into the tree t, unless it's already there.    */
/* Return a pointer to the resulting tree.                 */
    xl_Tree * n;
    int dir;

    n = (xl_Tree *) calloc_2(1, sizeof (xl_Tree));
    if (n == NULL) {
	fprintf(stderr, "xl_insert: ran out of memory, exiting.\n");
	exit(255);
    }
    n->item = strcpy(malloc_2(strlen(i)+1), i);
    if(trans) n->trans = strcpy(malloc_2(strlen(trans)+1), trans);

    if (t == NULL) {
	n->left = n->right = NULL;
	return n;
    }
    t = xl_splay(i,t);
    dir = strcasecmp(i,t->item);
    if (dir<0) {
	n->left = t->left;
	n->right = t;
	t->left = NULL;
	return n;
    } else if (dir>0) {
	n->right = t->right;
	n->left = t;
	t->right = NULL;
	return n;
    } else { /* We get here if it's already in the tree */
             /* Don't add it again                      */
	if(n->trans) free_2(n->trans);
	free_2(n->item);
	free_2(n);
	return t;
    }
}

xl_Tree * xl_delete(char *i, xl_Tree * t) {
/* Deletes i from the tree if it's there.               */
/* Return a pointer to the resulting tree.              */
    xl_Tree * x;
    if (t==NULL) return NULL;
    t = xl_splay(i,t);
    if (strcmp(i, t->item) == 0) {               /* found it */
        if (t->left == NULL) {
            x = t->right;
        } else {
            x = xl_splay(i, t->left);
            x->right = t->right;
        }
        if(t->trans) free_2(t->trans);
        free_2(t->item);
        free_2(t);
        return x;
    }
    return t;                         /* It wasn't there */
}


/************************ splay ************************/


void init_filetrans_data(void)
{
int i;

if(!GLOBALS->filesel_filter) { GLOBALS->filesel_filter = calloc_2(FILE_FILTER_MAX+1, sizeof(char *)); }
if(!GLOBALS->xl_file_filter) { GLOBALS->xl_file_filter = calloc_2(FILE_FILTER_MAX+1, sizeof(struct xl_tree_node *)); }

for(i=0;i<FILE_FILTER_MAX+1;i++)
	{
	GLOBALS->filesel_filter[i] = NULL;
	GLOBALS->xl_file_filter[i] = NULL;
	}
}


static void remove_file_filter_2(xl_Tree *t)
{
if(t->left) remove_file_filter_2(t->left);
if(t->right) remove_file_filter_2(t->right);
if(t->item) free_2(t->item);
if(t->trans) free_2(t->trans);
free_2(t);
}

static void remove_file_filter(int which, int regen)
{
if(GLOBALS->xl_file_filter[which])
	{
	remove_file_filter_2(GLOBALS->xl_file_filter[which]);
	GLOBALS->xl_file_filter[which] = NULL;
	}

if(regen)
        {
	GLOBALS->signalwindow_width_dirty=1;
	redraw_signals_and_waves();
        }
}

static void load_file_filter(int which, char *name)
{
FILE *f = fopen(name, "rb");
if(!f)
	{
	status_text("Could not open filter file!\n");
	return;
	}

remove_file_filter(which, 0); /* should never happen from GUI, but possible from save files or other weirdness */

while(!feof(f))
	{
	char *s = fgetmalloc(f);
	if(s)
		{
		char *lhs = s;

		while(*lhs && isspace((int)(unsigned char)*lhs)) lhs++;
		if(lhs)
			{
			char *rhs = lhs;

			if(*lhs != '#')	/* ignore comments */
				{
				while(*rhs && !isspace((int)(unsigned char)*rhs)) rhs++;
				if(*rhs)
					{
					char *xlt = rhs+1;
					*rhs = 0;

					while(*xlt && isspace((int)(unsigned char)*xlt)) xlt++;
					if(*xlt)
						{
						GLOBALS->xl_file_filter[which] =  xl_insert(lhs, GLOBALS->xl_file_filter[which], xlt);
						}
					}
				}
			}

		free_2(s);
		}
	}

fclose(f);
}


static void load_enums_filter(int which, char *name)
{
int argc;
char **spl = zSplitTclList(name, &argc);
int i;

if((!spl)||(!argc)||(argc&1))
	{
	status_text("Malformed enums list!\n");
	return;
	}

remove_file_filter(which, 0); /* should never happen from GUI, but possible from save files or other weirdness */

for(i=0;i<argc;i+=2)
	{
	char *lhs = spl[i];
	char *xlt = spl[i+1];

	GLOBALS->xl_file_filter[which] =  xl_insert(lhs, GLOBALS->xl_file_filter[which], xlt);
	}
free_2(spl);
}


int install_file_filter(int which)
{
int found = 0;

if((which<0)||(which>=(FILE_FILTER_MAX+1)))
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
                                t->f_filter = which;
				t->p_filter = 0;
				if(!which)
					{
					t->flags &= (~(TR_FTRANSLATED|TR_PTRANSLATED|TR_ANALOGMASK));
					}
					else
					{
					t->flags &= (~(TR_ANALOGMASK));
					t->flags |= TR_FTRANSLATED;
					}
                                found++;
                                }
                        }
                t=t->t_next;
                }
        }

if(found)
	{
	GLOBALS->signalwindow_width_dirty=1;
	redraw_signals_and_waves();
	}

return(found);
}

/************************************************************************/



static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

GLOBALS->is_active_translate_c_5=0;
gtk_widget_destroy(GLOBALS->window_translate_c_11);
GLOBALS->window_translate_c_11 = NULL;
}

static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
install_file_filter(GLOBALS->current_filter_translate_c_2);
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
	for(i=0;i<GLOBALS->num_file_filters;i++)
		{
		if(GLOBALS->filesel_filter[i])
			{
			if(!strcmp(GLOBALS->filesel_filter[i], *GLOBALS->fileselbox_text))
				{
				status_text("Filter already imported.\n");
				if(GLOBALS->is_active_translate_c_5) gdk_window_raise(gtk_widget_get_window(GLOBALS->window_translate_c_11));
				return;
				}
			}
		}
	}

GLOBALS->num_file_filters++;
load_file_filter(GLOBALS->num_file_filters, *GLOBALS->fileselbox_text);
if(GLOBALS->xl_file_filter[GLOBALS->num_file_filters] && (*GLOBALS->fileselbox_text /* scan-build */))
	{
	if(GLOBALS->filesel_filter[GLOBALS->num_file_filters]) free_2(GLOBALS->filesel_filter[GLOBALS->num_file_filters]);
	GLOBALS->filesel_filter[GLOBALS->num_file_filters] = malloc_2(strlen(*GLOBALS->fileselbox_text) + 1);
	strcpy(GLOBALS->filesel_filter[GLOBALS->num_file_filters], *GLOBALS->fileselbox_text);

	GtkTreeIter iter;
        gtk_list_store_append (GTK_LIST_STORE(GLOBALS->sig_store_translate), &iter);
        gtk_list_store_set (GTK_LIST_STORE(GLOBALS->sig_store_translate), &iter,
                                    NAME_COLUMN, GLOBALS->filesel_filter[GLOBALS->num_file_filters],
                                    -1);
	}
	else
	{
	GLOBALS->num_file_filters--;
	}

if(GLOBALS->is_active_translate_c_5) gdk_window_raise(gtk_widget_get_window(GLOBALS->window_translate_c_11));
}

static void add_filter_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

if(GLOBALS->num_file_filters == FILE_FILTER_MAX)
	{
	status_text("Max number of file filters installed already.\n");
	return;
	}

fileselbox("Select Filter File",&GLOBALS->fcurr_translate_c_2,G_CALLBACK(add_filter_callback_2), G_CALLBACK(NULL),NULL, 0);
}

/*
 * mainline..
 */
void trans_searchbox(char *title)
{
    int i;

    GtkWidget *scrolled_win;
    GtkWidget *vbox1, *hbox, *hbox0;
    GtkWidget *button1, *button5, *button6;
    gchar *titles[]={"Filter Select"};
    GtkWidget *frame2, *frameh, *frameh0;
    GtkWidget *table;

    if(GLOBALS->is_active_translate_c_5)
	{
	gdk_window_raise(gtk_widget_get_window(GLOBALS->window_translate_c_11));
	return;
	}

    GLOBALS->is_active_translate_c_5=1;
    GLOBALS->current_filter_translate_c_2 = 0;

    /* create a new modal window */
    GLOBALS->window_translate_c_11 = gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window_translate_c_11, ((char *)&GLOBALS->window_translate_c_11) - ((char *)GLOBALS));

    gtk_window_set_title(GTK_WINDOW (GLOBALS->window_translate_c_11), title);
    gtkwave_signal_connect(XXX_GTK_OBJECT (GLOBALS->window_translate_c_11), "delete_event",(GCallback) destroy_callback, NULL);

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

    GLOBALS->sig_store_translate = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING);
    GtkWidget *sig_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (GLOBALS->sig_store_translate));

    /* The view now holds a reference.  We can get rid of our own reference */
    g_object_unref (G_OBJECT (GLOBALS->sig_store_translate));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (titles[0], renderer, "text", NAME_COLUMN, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (sig_view), column);

    /* Setup the selection handler */
    GLOBALS->sig_selection_translate = gtk_tree_view_get_selection (GTK_TREE_VIEW (sig_view));
    gtk_tree_selection_set_mode (GLOBALS->sig_selection_translate, GTK_SELECTION_SINGLE);
    gtk_tree_selection_set_select_function (GLOBALS->sig_selection_translate, XXX_view_selection_func, NULL, NULL);

    gtk_list_store_clear (GTK_LIST_STORE(GLOBALS->sig_store_translate));
    for(i=0;i<GLOBALS->num_file_filters;i++)
	{
	GtkTreeIter iter;
        gtk_list_store_append (GTK_LIST_STORE(GLOBALS->sig_store_translate), &iter);
        gtk_list_store_set (GTK_LIST_STORE(GLOBALS->sig_store_translate), &iter,
                                    NAME_COLUMN, GLOBALS->filesel_filter[i+1],
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

    button6 = gtk_button_new_with_label (" Add Filter to List ");
    gtk_container_set_border_width (GTK_CONTAINER (button6), 3);
    gtkwave_signal_connect_object (XXX_GTK_OBJECT (button6), "clicked",G_CALLBACK(add_filter_callback),XXX_GTK_OBJECT (GLOBALS->window_translate_c_11));
    gtk_widget_show (button6);
    gtk_tooltips_set_tip_2(button6,
		"Bring up a file requester to add a filter to the filter select window.");

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
    gtkwave_signal_connect_object (XXX_GTK_OBJECT (button1), "clicked",G_CALLBACK(ok_callback),XXX_GTK_OBJECT (GLOBALS->window_translate_c_11));
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
    gtkwave_signal_connect_object (XXX_GTK_OBJECT (button5), "clicked",G_CALLBACK(destroy_callback),XXX_GTK_OBJECT (GLOBALS->window_translate_c_11));
    gtk_tooltips_set_tip_2(button5,
		"Do nothing and return to the main window.");
    gtk_widget_show (button5);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_box_pack_start(GTK_BOX(hbox), button5, TRUE, TRUE, 0);
#else
    gtk_box_pack_start (GTK_BOX (hbox), button5, TRUE, FALSE, 0);
#endif

    gtk_container_add (GTK_CONTAINER (frameh), hbox);
    gtk_container_add (GTK_CONTAINER (GLOBALS->window_translate_c_11), table);

    gtk_widget_set_size_request(GTK_WIDGET(GLOBALS->window_translate_c_11), 400, 400);
    gtk_widget_show(GLOBALS->window_translate_c_11);
}


/*
 * currently only called by parsewavline+tcl
 */
static void set_current_translate_generic(char *name, int typ)
{
int i;

if(typ)
	{
	for(i=1;i<GLOBALS->num_file_filters+1;i++)
		{
		if(!strcmp(GLOBALS->filesel_filter[i], name)) { GLOBALS->current_translate_file = i; return; }
		}

	if(!strcmp(WAVE_TCL_INSTALLED_FILTER, name))
		{
		GLOBALS->current_translate_file = 0; return;
		}
	}

if(GLOBALS->num_file_filters < FILE_FILTER_MAX)
	{
	GLOBALS->num_file_filters++;

	if(typ)
		{
		load_file_filter(GLOBALS->num_file_filters, name);
		}
		else
		{
		load_enums_filter(GLOBALS->num_file_filters, name);
		}

	if(!GLOBALS->xl_file_filter[GLOBALS->num_file_filters])
		{
		GLOBALS->num_file_filters--;
		GLOBALS->current_translate_file = 0;
		}
		else
		{
		if(GLOBALS->filesel_filter[GLOBALS->num_file_filters]) free_2(GLOBALS->filesel_filter[GLOBALS->num_file_filters]);

		if(!typ)
			{
			name = WAVE_TCL_INSTALLED_FILTER;
			}

		GLOBALS->filesel_filter[GLOBALS->num_file_filters] = malloc_2(strlen(name) + 1);
		strcpy(GLOBALS->filesel_filter[GLOBALS->num_file_filters], name);
		GLOBALS->current_translate_file = GLOBALS->num_file_filters;
		}
	}
}


void set_current_translate_file(char *name)
{
set_current_translate_generic(name, 1); /* use file, not enums */
}

void set_current_translate_enums(char *lst)
{
set_current_translate_generic(lst, 0); /* use enums */
}


